#include "Include/ComputeControllers/ParallelSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"
#include "Include/SSBOs/IntermediateData.h" // for debugging (TODO: remove this when done)

#include "Shaders/ParallelSort/ParallelSortConstants.comp"
#include "Shaders/ComputeHeaders/UniformLocations.comp"


#include <chrono>
#include <algorithm>
#include <iostream>
using std::cout;
using std::endl;


/*------------------------------------------------------------------------------------------------
Description:
    Generates multiple compute shaders for the different stages of the parallel sort, and 
    allocates various buffers for the sorting.  Buffer sizes are highly dependent on the size of 
    the original data.  They are expected to remain constant after class creation.
    
    The original data's SSBO MUST be passed in so that the uniform specifying buffer size can be 
    set for any compute shaders that use it.
Parameters: 
    dataToSort  See Description.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
ParallelSort::ParallelSort(const OriginalDataSsbo::UNIQUE_PTR &dataToSort) :
    _originalDataToIntermediateDataProgramId(0),
    _getBitForPrefixScansProgramId(0),
    _parallelPrefixScanProgramId(0),
    _sortIntermediateDataProgramId(0),
    _originalDataCopySsbo(nullptr),
    _intermediateDataFirstBuffer(nullptr),
    _intermediateDataSecondBuffer(nullptr),
    _prefixSumSsbo(nullptr)
{
    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();
    std::string shaderKey;

    // take a data structure that needs to be sorted by a value (must be unsigned int for radix 
    // sort to work) and put it into an intermediate structure that has the value and the index 
    // of the original data structure in the OriginalDataBuffer
    shaderKey = "original data to intermediate data";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/OriginalDataBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/OriginalDataToIntermediateData.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _originalDataToIntermediateDataProgramId = shaderStorageRef.GetShaderProgram(shaderKey);
    dataToSort->ConfigureUniforms(_originalDataToIntermediateDataProgramId);

    // on each loop in Sort(), pluck out a single bit and add it to the 
    // PrefixScanBuffer::PrefixSumsWithinGroup array
    shaderKey = "get bit for prefix sums";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/GetBitForPrefixScan.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _getBitForPrefixScansProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    // run the prefix scan over PrefixScanBuffer::PrefixSumsWithinGroup, and after run the scan again 
    // over PrefixScanBuffer::PrefixSumsPerGroup
    shaderKey = "parallel prefix scan";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelPrefixScan.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _parallelPrefixScanProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    // and sort the "read" arry from IntermediateSortBuffers into the "write" array
    // REQUIRES Version.comp
    // REQUIRES ParallelSortConstants.comp
    // - PARALLEL_SORT_WORK_GROUP_SIZE_X
    // REQUIRES UniformLocations.comp
    // REQUIRES SsboBufferBindings.comp
    // REQUIRES IntermediateSortBuffers.comp
    // REQUIRES PrefixScanBuffer.comp
    shaderKey = "sort intermediate data";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/SortIntermediateData.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _sortIntermediateDataProgramId = shaderStorageRef.GetShaderProgram(shaderKey);



    unsigned int originalDataSize = dataToSort->NumItems();
    _originalDataCopySsbo = std::make_unique<OriginalDataCopySsbo>(originalDataSize);
    _prefixSumSsbo = std::make_unique<PrefixSumSsbo>(originalDataSize);
    _prefixSumSsbo->ConfigureUniforms(_getBitForPrefixScansProgramId);
    _prefixSumSsbo->ConfigureUniforms(_parallelPrefixScanProgramId);

    // see explanation in the PrefixSumSsbo constructor for why there are likely more entries in 
    // PrefixScanBuffer::PrefixSumsWithinGroup than the requested number of items that need sorting
    unsigned int numEntriesInPrefixSumBuffer = _prefixSumSsbo->NumDataEntries();
    _intermediateDataFirstBuffer = std::make_unique<IntermediateDataFirstBuffer>(numEntriesInPrefixSumBuffer);
    _intermediateDataFirstBuffer->ConfigureUniforms(_originalDataToIntermediateDataProgramId);
    _intermediateDataFirstBuffer->ConfigureUniforms(_getBitForPrefixScansProgramId);
    _intermediateDataSecondBuffer = std::make_unique<IntermediateDataSecondBuffer>(numEntriesInPrefixSumBuffer);

}

// TODO: header
void ParallelSort::Sort()
{
    // TODO: (if actually sorting original data) need an SsboBase pointer and need to copy the original data buffer to the original data copy buffer

    // sorting runs over an array (one dimension), so only need to calculate X work group size
    // Note: +1 to total so that a data set less than a single work group size still gets a work 
    // group. 
    // Also Note: +1 to work group size so that a data set size that is an exact multiple of the 
    // work group size doesn't get an extra work group.
    unsigned int numItemsInPrefixScanBuffer = _prefixSumSsbo->NumDataEntries();
    int numWorkGroupsXByWorkGroupSize = (numItemsInPrefixScanBuffer / (PARALLEL_SORT_WORK_GROUP_SIZE_X + 1)) + 1;
    int numWorkGroupsXByItemsPerWorkGroup = (numItemsInPrefixScanBuffer / (ITEMS_PER_WORK_GROUP + 1)) + 1;
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    using namespace std::chrono;
    // give the counters initial data
    steady_clock::time_point start;
    steady_clock::time_point end;


    printf("1\n");

    // moving original data to intermediate data is 1 item per thread
    start = high_resolution_clock::now();
    glUseProgram(_originalDataToIntermediateDataProgramId);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    printf("2\n");

    // copy the data back and check
    std::vector<IntermediateData> checkUnsortedIntermediateData(numItemsInPrefixScanBuffer);
    unsigned int bufferSizeBytes = checkUnsortedIntermediateData.size() * sizeof(IntermediateData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    memcpy(checkUnsortedIntermediateData.data(), bufferPtr, bufferSizeBytes);

    printf("3\n");


    // getting 1 bit value from intermediate data to prefix sum is 1 item per thread
    start = high_resolution_clock::now();
    glUseProgram(_getBitForPrefixScansProgramId);
    unsigned int readFromFirstIntermediateBuffer = 1;
    unsigned int bitOffset = 0;
    glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readFromFirstIntermediateBuffer);
    glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitOffset);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    // TODO: ??would this work fast (and still work right) if the memory barrier were removed??
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    printf("4\n");

    // copy the data back and check
    // Note: The +1 is because of totalNumberOfOnes.
    unsigned int totalPrefixSumItems = ITEMS_PER_WORK_GROUP + 1 + numItemsInPrefixScanBuffer;
    std::vector<unsigned int> checkPrefixScanBufferPreScan(totalPrefixSumItems);
    bufferSizeBytes = checkPrefixScanBufferPreScan.size() * sizeof(unsigned int);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkPrefixScanBufferPreScan.data(), bufferPtr, bufferSizeBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    printf("5\n");

    // parallel prefix scan is 2 items per thread
    start = high_resolution_clock::now();
    glUseProgram(_parallelPrefixScanProgramId);

    // prefix scan over all values
    glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 1);
    glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // prefix scan over per-work-group sums
    // Note: The PrefixSumsPerGroup array is sized to be exactly enough for 1 work group.  It 
    // makes the prefix sum easier than trying to eliminate excess threads.
    glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 0);
    glDispatchCompute(1, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // copy the data back and check
    std::vector<unsigned int> checkPrefixScanBufferPostScan(totalPrefixSumItems);
    bufferSizeBytes = checkPrefixScanBufferPostScan.size() * sizeof(unsigned int);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkPrefixScanBufferPostScan.data(), bufferPtr, bufferSizeBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    unsigned int manualPrefixSum = 0;
    for (unsigned int prefixSumIndex = 1; prefixSumIndex < _prefixSumSsbo->NumDataEntries(); prefixSumIndex++)
    {

    //}
    //for (unsigned int prefixSumIndex = ITEMS_PER_WORK_GROUP + 2; prefixSumIndex < checkPrefixScanBufferPostScan.size(); prefixSumIndex++)
    //{
        // Note: Remember to account for the "per work group sums" that comes before the individual prefix scans.
        // Also Note: Remember to account for uint totalNumberOfOnes.
        unsigned int indexToCheck = ITEMS_PER_WORK_GROUP + 1 + prefixSumIndex;



        unsigned int currentPrefixSum = checkPrefixScanBufferPostScan[indexToCheck];

        if (prefixSumIndex % ITEMS_PER_WORK_GROUP == 0)
        {
            // reset
            manualPrefixSum = 0;
        }
        else
        {
            manualPrefixSum += checkPrefixScanBufferPreScan[indexToCheck - 1];
        }

        if (currentPrefixSum != manualPrefixSum)
        {
            printf("");
        }
    }

    printf("6\n");

    // and sort the intermediate data with the scanned values
    glUseProgram(_sortIntermediateDataProgramId);
    glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readFromFirstIntermediateBuffer);
    glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitOffset);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    // TODO: ??would this work fast (and still work right) if the memory barrier were removed??
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // copy the data back and check
    std::vector<IntermediateData> checkSortedIntermediateData(numItemsInPrefixScanBuffer);
    bufferSizeBytes = checkSortedIntermediateData.size() * sizeof(IntermediateData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    memcpy(checkSortedIntermediateData.data(), bufferPtr, bufferSizeBytes);



    //end = high_resolution_clock::now();
    //cout << "verifying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    printf("");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

