#include "Include/ComputeControllers/ParticleSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"



#include <array>



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


#include <random>

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




    glUseProgram(_parallelPrefixScanComputeProgramId);

    // TODO: replace 512 with a constant
    int perGroupSumsSize = 512 * 2;
    int intArrByteOffset = perGroupSumsSize * sizeof(unsigned int);
    std::vector<unsigned int> dataToSum = { 0,1,0,0,1,0,1,0,1,0,0,1,1,0,0,1 };
    int totalItems = perGroupSumsSize + dataToSum.size();
    int bufferSizeBytes = totalItems * sizeof(unsigned int);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, intArrByteOffset, dataToSum.size() * sizeof(unsigned int), dataToSum.data());

    // TODO: replace hard-coded 512 with a constant from a header/compute-kosher include file
    int numWorkGroupsX = (numParticles / 512) + 1;
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    //unsigned int maxThreads = numParticles / 2;
    unsigned int maxThreads = numWorkGroupsX * 512;
    glUniform1ui(_unifLocMaxThreadCount, maxThreads);



    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    std::vector<unsigned int> v(totalItems);
    memcpy(v.data(), bufferPtr, bufferSizeBytes);

    // check the GPU's results
    unsigned int manualPrefixSum = 0;
    for (unsigned int prefixSumIndex = 1; prefixSumIndex < dataToSum.size(); prefixSumIndex++)
    {
        unsigned int currentPrefixSum = v[perGroupSumsSize + prefixSumIndex];
        manualPrefixSum += dataToSum[prefixSumIndex - 1];

        if (currentPrefixSum != manualPrefixSum)
        {
            printf("");
        }
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

