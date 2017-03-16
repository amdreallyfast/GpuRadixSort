#include "Include/ComputeControllers/ParticleSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"

#include <chrono>
#include <algorithm>
#include <iostream>
using std::cout;
using std::endl;


// TODO: header
ParticleSort::ParticleSort(unsigned int numParticles) :
    _prefixSumSsbo(nullptr),
    _parallelPrefixScanComputeProgramId(0),
    _unifLocMaxThreadCount(0)
{
    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    // creating Morton Code/particle pairs and putting them into a buffer that needs sorting
    // Note: The sorting goes bit by bit, so over a 30bit Morton Code these things will need to be moved 30 times.  These little structures are easier to move than the particle structures.

    // for i = 0 to 29
    //  use bit offset i
    //  copy ith bit of Morton Code into a shared memory array, generate prefix scans for workgroups, and generate per-workgroup sums
    //  generate prefix scan for per-workgroup sums
    //  move Morton Code/particle pairs around according to the workgroup prefix scans and per-workgroup sums
    std::string parallelPrefixScanKey = "parallel prefix scan";
    shaderStorageRef.NewCompositeShader(parallelPrefixScanKey);
    shaderStorageRef.AddPartialShaderFile(parallelPrefixScanKey, "Shaders/ParallelPrefixScan.comp");
    shaderStorageRef.CompileCompositeShader(parallelPrefixScanKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(parallelPrefixScanKey);
    _parallelPrefixScanComputeProgramId = shaderStorageRef.GetShaderProgram(parallelPrefixScanKey);

    _unifLocMaxThreadCount = shaderStorageRef.GetUniformLocation(parallelPrefixScanKey, "uMaxThreadCount");


    _prefixSumSsbo = new PrefixSumSsbo(numParticles);
    _prefixSumSsbo->ConfigureCompute(_parallelPrefixScanComputeProgramId, "IntBuffer");

    
    


    // copy particles to backup buffer
    // copy particles to sorted positions (Morton Code/particle pair indexes)
}

// TODO: header
ParticleSort::~ParticleSort()
{
    delete _prefixSumSsbo;
}

// TODO: header
void ParticleSort::Sort(int numParticles)
{
    // TODO: need uniform "max thread count"
    // TODO: if global thread ID > "max thread count" return
    // TODO: num work groups = (num particles / (work group size * 2)) + 1
    // TODO: "max thread count" = (num particles / 2)
    // Ex: num particles = 42, work group size = 512
    //  Each thread handles two elements, and prefix sums are calculated for an entire work group
    //  num work groups = (42 / (512 * 2)) + 1 
    //                  = (42 / 1024) + 1
    //                  = 0 + 1
    //                  = 1
    //  max thread count    = (42 / 2)
    //                      = 21
    // In this example, threads 22 - 511 will have no data to work on.  Threads 0 - 20 will operate on the 42 items (thread 0 on indices 0 and 1, thread 1 on indices 1 and 2, ..., thread 20 on indices 40 and 41).  The shared memory structure is already initialized to 0s, so these threads should just return.

    // Ex: num particles = 2500, work group size = 512
    //  Each thread handles 2 elements.
    //  num work groups = (2500 / (512 * 2)) + 1
    //                  = (2500 / 1024) + 1
    //                  = 2 + 1
    //                  = 3
    //  max thread count    = (2500 / 2) + 1
    //                      = 1250
    //                      = 1250
    // In this example, 512 threads (0 - 511) calculate the prefix sum for work group 0 (indices 0 - 1023), 512 threads (512 - 1023) calculate the pregix sum for work group 1 (indices 1024 - 2047), and 226 threads (1024 - 1249) calculate the prefix sum for work group 2 (indices 2048 - 2499).  The rest of the threads in work group 2 ((512 * 3) - 1250) = 286 threads (1250 - 1536) will have nothing to do and should simply return.

    using namespace std::chrono;
    // give the counters initial data
    steady_clock::time_point start;
    steady_clock::time_point end;

    start = high_resolution_clock::now();
    glUseProgram(_parallelPrefixScanComputeProgramId);

    // TODO: replace 512 with a constant
    int threadsPerWorkGroup = 512;
    int dataSizePerWorkGroup = threadsPerWorkGroup * 2;
    int intArrByteOffset = dataSizePerWorkGroup * sizeof(unsigned int);
    std::vector<unsigned int> dataToSum;
    for (int i = 0; i < numParticles; i++)
    {
        // this will push back an alternating assortment of 0s and 1s
        dataToSum.push_back(i % 2);
    }
    std::random_shuffle(dataToSum.begin(), dataToSum.end());

    end = high_resolution_clock::now();
    cout << "generating " << numParticles << " numbers: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    start = high_resolution_clock::now();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, intArrByteOffset, dataToSum.size() * sizeof(unsigned int), dataToSum.data());
    end = high_resolution_clock::now();
    cout << "copying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    // TODO: replace hard-coded 512 with a constant from a header/compute-kosher include file
    // Note: Divide by "data size per work group" + 1 because, if the number of items == "data size per work group", then numWorkGroupsX = (X/X) + 1 = 2.  This is a corner case, but it needs to be accounted for.
    int numWorkGroupsX = (numParticles / (dataSizePerWorkGroup + 1)) + 1;
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    //unsigned int maxThreads = numParticles / 2;
    unsigned int maxThreads = numWorkGroupsX * threadsPerWorkGroup;
    glUniform1ui(_unifLocMaxThreadCount, maxThreads);

    
    start = high_resolution_clock::now();
    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    end = high_resolution_clock::now();
    cout << "running prefix scan with " << maxThreads << " threads over " << numWorkGroupsX << " work groups: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    //int totalItems = dataSizePerWorkGroup + dataToSum.size();
    int totalItems = dataSizePerWorkGroup + (numWorkGroupsX * dataSizePerWorkGroup);
    int bufferSizeBytes = totalItems * sizeof(unsigned int);

    start = high_resolution_clock::now();
    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    std::vector<unsigned int> v(totalItems);
    memcpy(v.data(), bufferPtr, bufferSizeBytes);
    end = high_resolution_clock::now();
    cout << "copying data back: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    // check the GPU's results
    start = high_resolution_clock::now();
    unsigned int manualPrefixSum = 0;
    for (unsigned int prefixSumIndex = 1; prefixSumIndex < dataToSum.size(); prefixSumIndex++)
    {
        // remember to account for the "per work group sums" that comes before the individual prefix scans
        unsigned int currentPrefixSum = v[dataSizePerWorkGroup + prefixSumIndex];

        if (prefixSumIndex % dataSizePerWorkGroup == 0)
        {
            // reset
            manualPrefixSum = 0;
        }
        else
        {
            manualPrefixSum += dataToSum[prefixSumIndex - 1];
        }

        if (currentPrefixSum != manualPrefixSum)
        {
            printf("");
        }
    }

    end = high_resolution_clock::now();
    cout << "verifying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

