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
    _prefixSumSsbo->ConfigureUniforms(_sortIntermediateDataProgramId);

    // see explanation in the PrefixSumSsbo constructor for why there are likely more entries in 
    // PrefixScanBuffer::PrefixSumsWithinGroup than the requested number of items that need sorting
    unsigned int numEntriesInPrefixSumBuffer = _prefixSumSsbo->NumDataEntries();
    _intermediateDataFirstBuffer = std::make_unique<IntermediateDataFirstBuffer>(numEntriesInPrefixSumBuffer);
    _intermediateDataFirstBuffer->ConfigureUniforms(_originalDataToIntermediateDataProgramId);
    _intermediateDataFirstBuffer->ConfigureUniforms(_getBitForPrefixScansProgramId);
    _intermediateDataFirstBuffer->ConfigureUniforms(_sortIntermediateDataProgramId);
    _intermediateDataSecondBuffer = std::make_unique<IntermediateDataSecondBuffer>(numEntriesInPrefixSumBuffer);

}

#include <math.h>
static void printRepeating(std::vector<IntermediateData> &arr)
{
    int i;
    printf("The repeating elements are: \n");
    for (i = 0; i < arr.size(); i++)
    {
        if (arr[i]._data == -1)
        {
            continue;
        }

        int temp = arr[i]._data;
        temp = abs(temp);
        temp = arr[temp]._data;

        int index = arr[i]._data;
        index = abs(index);
        int val = arr[index]._data;
        if (val >= 0)
            arr[index]._data = -arr[index]._data;
        else
            printf(" - %d at index %d\n", abs(val), index);
    }
}

static void FindIndex(std::vector<IntermediateData> &arr, unsigned int value)
{
    for (size_t i = 0; i < arr.size(); i++)
    {
        if (arr[i]._data == value)
        {
            printf("found value %u at index %u\n", value, i);
        }
    }
}

static void FindIndex(std::vector<unsigned int> &arr, unsigned int value)
{
    for (size_t i = 0; i < arr.size(); i++)
    {
        if (arr[i] == value)
        {
            printf("found value %u at index %u\n", value, i);
        }
    }
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


    //printf("1\n");

    //// moving original data to intermediate data is 1 item per thread
    //start = high_resolution_clock::now();
    //glUseProgram(_originalDataToIntermediateDataProgramId);
    //glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    //printf("2\n");

    //// copy the data back and check
    //std::vector<IntermediateData> checkIntermediateReadBuffer(numItemsInPrefixScanBuffer);
    //unsigned int bufferSizeBytes = checkIntermediateReadBuffer.size() * sizeof(IntermediateData);
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    //void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    //memcpy(checkIntermediateReadBuffer.data(), bufferPtr, bufferSizeBytes);

    //printf("3\n");


    //// getting 1 bit value from intermediate data to prefix sum is 1 item per thread
    //start = high_resolution_clock::now();
    //glUseProgram(_getBitForPrefixScansProgramId);
    //unsigned int readFromFirstIntermediateBuffer = 1;
    //unsigned int bitOffset = 0;
    //glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readFromFirstIntermediateBuffer);
    //glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitOffset);
    //glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    //// TODO: ??would this work fast (and still work right) if the memory barrier were removed??
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    //printf("4\n");

    //// copy the data back and check
    //// Note: The +1 is because of totalNumberOfOnes.
    //unsigned int totalPrefixSumItems = ITEMS_PER_WORK_GROUP + 1 + numItemsInPrefixScanBuffer;
    //std::vector<unsigned int> checkPrefixScanBufferPreScan(totalPrefixSumItems);
    //bufferSizeBytes = checkPrefixScanBufferPreScan.size() * sizeof(unsigned int);
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //memcpy(checkPrefixScanBufferPreScan.data(), bufferPtr, bufferSizeBytes);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    //printf("5\n");

    //// parallel prefix scan is 2 items per thread
    //start = high_resolution_clock::now();
    //glUseProgram(_parallelPrefixScanProgramId);

    //// prefix scan over all values
    //glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 1);
    //glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    //// prefix scan over per-work-group sums
    //// Note: The PrefixSumsPerGroup array is sized to be exactly enough for 1 work group.  It 
    //// makes the prefix sum easier than trying to eliminate excess threads.
    //glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 0);
    //glDispatchCompute(1, numWorkGroupsY, numWorkGroupsZ);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    //// copy the data back and check
    //std::vector<unsigned int> checkPrefixScanBufferPostScan(totalPrefixSumItems);
    //bufferSizeBytes = checkPrefixScanBufferPostScan.size() * sizeof(unsigned int);
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //memcpy(checkPrefixScanBufferPostScan.data(), bufferPtr, bufferSizeBytes);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    //unsigned int manualPrefixSum = 0;
    //for (unsigned int prefixSumIndex = 1; prefixSumIndex < _prefixSumSsbo->NumDataEntries(); prefixSumIndex++)
    //{

    ////}
    ////for (unsigned int prefixSumIndex = ITEMS_PER_WORK_GROUP + 2; prefixSumIndex < checkPrefixScanBufferPostScan.size(); prefixSumIndex++)
    ////{
    //    // Note: Remember to account for the "per work group sums" that comes before the individual prefix scans.
    //    // Also Note: Remember to account for uint totalNumberOfOnes.
    //    unsigned int indexToCheck = ITEMS_PER_WORK_GROUP + 1 + prefixSumIndex;



    //    unsigned int currentPrefixSum = checkPrefixScanBufferPostScan[indexToCheck];

    //    if (prefixSumIndex % ITEMS_PER_WORK_GROUP == 0)
    //    {
    //        // reset
    //        manualPrefixSum = 0;
    //    }
    //    else
    //    {
    //        manualPrefixSum += checkPrefixScanBufferPreScan[indexToCheck - 1];
    //    }

    //    if (currentPrefixSum != manualPrefixSum)
    //    {
    //        printf("");
    //    }
    //}

    //printf("6\n");

    //// and sort the intermediate data with the scanned values
    //glUseProgram(_sortIntermediateDataProgramId);
    //glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readFromFirstIntermediateBuffer);
    //glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitOffset);
    //glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    //// TODO: ??would this work fast (and still work right) if the memory barrier were removed??
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    //// copy the data back and check
    //std::vector<IntermediateData> checkIntermediateWriteBuffer(numItemsInPrefixScanBuffer);
    //bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
    //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    //memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);


    //steady_clock::duration originalDataToIntermediateData;
    //steady_clock::duration getBitForPrefixScan;
    //steady_clock::duration prefixScanAllData;
    //steady_clock::duration prefixScanGroupData;
    //steady_clock::duration sortIntermediateData;


unsigned int bufferSizeBytes = 0;
void *bufferPtr = 0;

unsigned int totalPrefixSumItems = ITEMS_PER_WORK_GROUP + 1 + numItemsInPrefixScanBuffer;
std::vector<unsigned int> checkPrefixScanBufferPreScan(totalPrefixSumItems);
std::vector<unsigned int> checkPrefixScanBufferPostScan(totalPrefixSumItems);
std::vector<IntermediateData> checkIntermediateReadBuffer(numItemsInPrefixScanBuffer);
std::vector<IntermediateData> checkIntermediateWriteBuffer(numItemsInPrefixScanBuffer);


    // moving original data to intermediate data is 1 item per thread
    start = high_resolution_clock::now();
    glUseProgram(_originalDataToIntermediateDataProgramId);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize + 1, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);



    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    //bufferSizeBytes = checkIntermediateReadBuffer.size() * sizeof(IntermediateData);
    //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //memcpy(checkIntermediateReadBuffer.data(), bufferPtr, bufferSizeBytes);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    printRepeating(checkIntermediateReadBuffer);

    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
    //bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
    //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);



    bool readFromFirstIntermediateBuffer = true;

    // for 32bit unsigned integers, make 32 passes
    for (unsigned int bitNumber = 0; bitNumber < 32; bitNumber++)
    {
        unsigned int readBufferNumber = (unsigned int)readFromFirstIntermediateBuffer;


        // getting 1 bit value from intermediate data to prefix sum is 1 item per thread
        glUseProgram(_getBitForPrefixScansProgramId);
        glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readBufferNumber);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByWorkGroupSize + 1, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //// copy the data back and check
        //bufferSizeBytes = checkPrefixScanBufferPreScan.size() * sizeof(unsigned int);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
        //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //memcpy(checkPrefixScanBufferPreScan.data(), bufferPtr, bufferSizeBytes);
        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);





        // parallel prefix scan is 2 items per thread
        glUseProgram(_parallelPrefixScanProgramId);

        // prefix scan over all values
        glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 1);
        glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //// copy the data back and check (using "pre scan" to refer to prior to per-work-group sums)
        //bufferSizeBytes = checkPrefixScanBufferPostScan.size() * sizeof(unsigned int);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
        //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //memcpy(checkPrefixScanBufferPostScan.data(), bufferPtr, bufferSizeBytes);
        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        //FindIndex(checkPrefixScanBufferPreScan, 131072);
        //FindIndex(checkPrefixScanBufferPostScan, 131072);

        // prefix scan over per-work-group sums
        // Note: The PrefixSumsPerGroup array is sized to be exactly enough for 1 work group.  It 
        // makes the prefix sum easier than trying to eliminate excess threads.
        glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 0);
        glDispatchCompute(1, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //// copy the data back and check
        //bufferSizeBytes = checkPrefixScanBufferPostScan.size() * sizeof(unsigned int);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
        //bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //memcpy(checkPrefixScanBufferPostScan.data(), bufferPtr, bufferSizeBytes);
        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);




        // and sort the intermediate data with the scanned values
        glUseProgram(_sortIntermediateDataProgramId);
        glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readBufferNumber);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByWorkGroupSize + 1, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);




        //// copy the data back and check
        //if (readFromFirstIntermediateBuffer)
        //{
        //    // read from first buffer, written to second
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
        //    bufferSizeBytes = checkIntermediateReadBuffer.size() * sizeof(IntermediateData);
        //    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //    memcpy(checkIntermediateReadBuffer.data(), bufferPtr, bufferSizeBytes);
        //    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
        //    bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
        //    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //    memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);
        //    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        //}
        //else
        //{
        //    // read from second buffer, written to first 
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
        //    bufferSizeBytes = checkIntermediateReadBuffer.size() * sizeof(IntermediateData);
        //    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //    memcpy(checkIntermediateReadBuffer.data(), bufferPtr, bufferSizeBytes);
        //    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); 

        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
        //    bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
        //    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
        //    memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);
        //    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        //}

        //size_t startIndex = 256 * 512;
        //size_t endIndex = 257 * 512;
        //unsigned int counter = 0;
        //for (size_t intCounter = startIndex; intCounter < endIndex; intCounter++)
        //{
        //    if (checkPrefixScanBufferPreScan[intCounter] != 0)
        //    {
        //        counter++;
        //    }
        //}

        // now switch intermediate buffers and do it again
        readFromFirstIntermediateBuffer = !readFromFirstIntermediateBuffer;

        //printf("Bit number %d: ", bitNumber);
        //printRepeating(checkIntermediateWriteBuffer);
        //FindIndex(checkIntermediateWriteBuffer, 65537);

        printf("-\n");
    }

    // copy the data back and check

    // undo the last buffer switch at the end of the last loop
    //readFromFirstIntermediateBuffer = !readFromFirstIntermediateBuffer;
    if (readFromFirstIntermediateBuffer)
    {
        // the sorted data wound up in the first buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    }
    else
    {
        // the sorted data wound up in the second buffer
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
    }

    bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
    bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // check
    for (size_t i = 1; i < checkIntermediateWriteBuffer.size(); i++)
    {
        unsigned int val = checkIntermediateWriteBuffer[i]._data;
        unsigned int prevVal = checkIntermediateWriteBuffer[i - 1]._data;

        if (val != prevVal + 1)
        {
            printf("");
        }
    }

    printf("");







    //end = high_resolution_clock::now();
    //cout << "verifying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    printf("");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}
