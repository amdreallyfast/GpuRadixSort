#include "Include/ComputeControllers/ParticleSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"
#include "Shaders/ComputeHeaders/ParallelSortConstants.comp"

#include <chrono>
#include <algorithm>
#include <iostream>
using std::cout;
using std::endl;


/*------------------------------------------------------------------------------------------------
Description:
    This is a trivial function, but it was put off on its own simple so that the extensive 
    explanation could be separated from its use in the ParallelPrefixScan.comp shader and in 
    ParticleSort::Sort(...).

    ----------------------------------------------------------------------------------------------
    This algorithm has each thread working on 2 data items, so the usual uMaxDataCount uniform 
    doesn't have much use as a thread check, but a "max thread count" does.  This value is 
    determined in ParticleSort::Sort().  

    "Max thread count" = num work groups * threads per work group

    Problem: The algorithm relies on a binary tree within a work group's data set, so the number 
    of items that are being summed within a work group must be a power of 2.  Each thread works 
    on 2 items, so the number of threads per work group is also a power of 2 
    (work group size = 256/512/etc.).  We humans like to have our data sets as multiples of 10 
    though, so data set sizes often don't divide evenly by a work group size (256/512/etc.).  

    Solution: Make the variable data set sizes work with the algorithm's reliance on a power of 
    2 by padding out the last work group's data set with 0s, and then let the full number of 
    threads per work group scan the data.  See PrefixSumSsbo for how it pads 0s onto the end so 
    that the last work group can run a full complement of threads.

    Is this wasting threads?  A little.  Only the last work group's data will be padded with 
    zeros, and in a large data set (100,000+) there will be dozens of work groups, if not 
    hundreds, so I won't concern myself with trying to optimize that last group's threads.  It 
    is easier to just provide it with 0s to chew on.  Like hay for horses.  It's cheap.

    In the following examples, work group size = 512, so 
    ITEMS_PER_WORK_GROUP = work group size * 2 = 1024.
    Note: Explanation for the +1 in the denominator given in Ex 2.

    Ex 1: data size = 42
    num work groups = (42 / (1024 + 1)) + 1 
                    = (42 / 1025) + 1
                    = 0 + 1
                    = 1
    allocated data  = 1 * ITEMS_PER_WORK_GROUP
                    = 1 * 1024
                    = 1024
    max thread count    = 1 * 512
                        = 512 (threads 0 - 511)
    In this example, there are only 42 items work looking at, so only 21 threads (0 - 20) will 
    be busy doing something useful, but to make the thread count cutoff easier to calculate, the 
    full work group complement of threads will be allowed to run and it is the PrefixSumSsbo's 
    responsibility to make sure that excess data is filled with 0s.

    Ex 2: data size = 1024
    num work groups = (1024 / ((512 * 2) + 1)) + 1
                    = (1024 / 1025) + 1
                    = 0 + 1
                    = 1
    allocated data  = 1 * ITEMS_PER_WORK_GROUP
                    = 1 * 1024
                    = 1024
    max thread count    = 1 * 512
                        = 512 (threads 0 - 511)
    The +1 in the denominator is to handle the case when the data set is exactly a multiple 1 
    work group.  

    Ex 3: data size = 2500
    num work groups = (2500 / (1025 + 1)) + 1
                    = (2500 / 1025) + 1
                    = 2 + 1
                    = 3
    allocated data  = 3 * ITEMS_PER_WORK_GROUP
                    = 3 * 1024
                    = 3072
    max thread count    = 3 * 512
                        = 1536 (threads 0 - 1535)
    In this example:
    - 512 threads (0 - 511) calculate the prefix sum for work group 0 (indices 0 - 1023)
    - 512 threads (512 - 1023) calculate the pregix sum for work group 1 (indices 1024 - 2047)
    - 512 threads (1024 - 1535) calculate the prefix sum for work group 2 (indices 2048 - 2499)
    The 572 excess allocated items (allocated data - data size) will be initialized to 0, so the 
    threads working on those items will technically be doing useless work, but it is just one 
    work group and it is easier to allocate excess data than it is to try to optimize which 
    threads are doing something useful.

    Ex 4: data size = 100,000
    num work groups = (100,000 / (1024 + 1)) + 1
                    = (100,000 / 1025) + 1
                    = 97 + 1
                    = 98
    allocated data  = 98 * ITEMS_PER_WORK_GROUP 
                    = 98 * 1024
                    = 100,352
    max thread count    = 98 * 512
                        = 50176 (threads 0 - 50175)
    In this example there are 352 excess items allocated, which means 176 threads that are 
    dealing with nothing but 0s.  This is a pittance compared to the total data size.
Parameters: 
    numWorkGroups   Num X work groups.  Y and Z are always 1.
Returns:    
    The maximum number of threads allowed in the whole invocation of the ParallelPrefixScan 
    shader.
Creator:    John Cox, 3-16-2017
------------------------------------------------------------------------------------------------*/
static unsigned int CalculateMaxThreadCount(unsigned int numWorkGroups)
{
    // TODO: replace the hard-coded 512 with values from a #define
    return numWorkGroups * 512;
}

// TODO: header
ParticleSort::ParticleSort(unsigned int numParticles) :
    _prefixSumSsbo(nullptr),
    _parallelPrefixScanComputeProgramId(0),
    _unifLocMaxThreadCount(0),
    _unifLocCalculateAll(0)
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
    shaderStorageRef.AddPartialShaderFile(parallelPrefixScanKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(parallelPrefixScanKey, "Shaders/ComputeHeaders/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(parallelPrefixScanKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(parallelPrefixScanKey, "Shaders/ParallelPrefixScan.comp");
    shaderStorageRef.CompileCompositeShader(parallelPrefixScanKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(parallelPrefixScanKey);
    _parallelPrefixScanComputeProgramId = shaderStorageRef.GetShaderProgram(parallelPrefixScanKey);

    _unifLocMaxThreadCount = shaderStorageRef.GetUniformLocation(parallelPrefixScanKey, "uMaxThreadCount");
    _unifLocCalculateAll = shaderStorageRef.GetUniformLocation(parallelPrefixScanKey, "uCalculateAll");

    _prefixSumSsbo = new PrefixSumSsbo(numParticles);
    _prefixSumSsbo->ConfigureCompute(_parallelPrefixScanComputeProgramId, "PrefixScanBuffer");

    
    


    // copy particles to backup buffer
    // copy particles to sorted positions (Morton Code/particle pair indexes)
}

// TODO: header
ParticleSort::~ParticleSort()
{
    delete _prefixSumSsbo;
}

// TODO: header
void ParticleSort::Sort(int /*numParticles*/)
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
    //int threadsPerWorkGroup = 512;
    //int dataSizePerWorkGroup = threadsPerWorkGroup * 2;
    //int intArrByteOffset = dataSizePerWorkGroup * sizeof(unsigned int);
    int dataSize = _prefixSumSsbo->NumDataEntries();
    int perGroupSumsDataSize = _prefixSumSsbo->NumPerGroupPrefixSums();
    int totalItems = perGroupSumsDataSize + dataSize;
    std::vector<unsigned int> dataToSum;
    //for (int i = 0; i < numParticles; i++)
    for (int i = 0; i < dataSize; i++)
    {
        // this will push back an alternating assortment of 0s and 1s
        dataToSum.push_back(i % 2);
    }
    std::random_shuffle(dataToSum.begin(), dataToSum.end());

    end = high_resolution_clock::now();
    cout << "generating " << dataSize << " numbers: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    start = high_resolution_clock::now();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    unsigned int offsetBytes = perGroupSumsDataSize * sizeof(unsigned int);
    unsigned int dataSizeBytes = dataToSum.size() * sizeof(unsigned int);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offsetBytes, dataSizeBytes, dataToSum.data());
    end = high_resolution_clock::now();
    cout << "copying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    // TODO: replace hard-coded 512 with a constant from a header/compute-kosher include file
    // Note: Divide by "data size per work group" + 1 because, if the number of items == "data size per work group", then numWorkGroupsX = (X/X) + 1 = 2.  This is a corner case, but it needs to be accounted for.
    int numWorkGroupsX = (dataSize / (ITEMS_PER_WORK_GROUP + 1)) + 1;
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    start = high_resolution_clock::now();

    // calculate all the prefix sums and fill out each work group's perGroupSums value
    glUniform1ui(_unifLocMaxThreadCount, CalculateMaxThreadCount(numWorkGroupsX));
    glUniform1ui(_unifLocCalculateAll, 1);    
    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);

    // now run the prefix scan over the perGroupSums array
    // Note: The perGroupSums array is only the size of 1 work group, so hard-code 1s.
    glUniform1ui(_unifLocMaxThreadCount, WORK_GROUP_SIZE_X);
    glUniform1ui(_unifLocCalculateAll, 0);
    glDispatchCompute(1, 1, 1);
     

    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    end = high_resolution_clock::now();
    cout << "running prefix scan with " << CalculateMaxThreadCount(numWorkGroupsX) << " threads over " << numWorkGroupsX << " work groups: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    //int totalItems = dataSizePerWorkGroup + dataToSum.size();
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
        unsigned int currentPrefixSum = v[ITEMS_PER_WORK_GROUP + prefixSumIndex];

        if (prefixSumIndex % ITEMS_PER_WORK_GROUP == 0)
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

