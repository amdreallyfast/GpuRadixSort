#include "Include/ComputeControllers/ParallelSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"
#include "Include/SSBOs/IntermediateData.h" // for debugging (TODO: remove this when done)

#include "Shaders/ParallelSort/ParallelSortConstants.comp"
#include "Shaders/ComputeHeaders/UniformLocations.comp"

#include <iostream>
#include <fstream>

#include <chrono>
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

    // TODO: SortOriginalData

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


/*------------------------------------------------------------------------------------------------
Description:
    This function is the main show of this demo.  It summons shaders to do the following:
    - Copy original data to intermediate data structures 
    - Get bits one at a time from the values in the intermediate data structures
    - Run the parallel prefix scan algorithm on those bit values
    - Sort the intermediate data structures using the resulting prefix sums
Parameters: None
Returns:    None
Creator:    John Cox
------------------------------------------------------------------------------------------------*/
void ParallelSort::Sort()
{
    // Note: See the explanation at the top of PrefixSumsSsbo.cpp for calculation explanation.
    unsigned int numItemsInPrefixScanBuffer = _prefixSumSsbo->NumDataEntries();

    // for ParallelPrefixScan.comp, which works on 2 items per thread
    int numWorkGroupsXByItemsPerWorkGroup = numItemsInPrefixScanBuffer / ITEMS_PER_WORK_GROUP;
    int remainder = numItemsInPrefixScanBuffer % ITEMS_PER_WORK_GROUP;
    numWorkGroupsXByItemsPerWorkGroup += (remainder == 0) ? 0 : 1;

    // for other shaders, which work on 1 item per thread
    int numWorkGroupsXByWorkGroupSize = numItemsInPrefixScanBuffer / PARALLEL_SORT_WORK_GROUP_SIZE_X;
    remainder = numItemsInPrefixScanBuffer % PARALLEL_SORT_WORK_GROUP_SIZE_X;
    numWorkGroupsXByWorkGroupSize += (remainder == 0) ? 0 : 1;

    // working on a 1D array (X dimension), so these are always 1
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    // for profiling
    using namespace std::chrono;
    steady_clock::time_point start;
    steady_clock::time_point end;
    long long durationOriginalDataToIntermediateData;
    long long durationDataVerification;
    std::vector<long long> durationsGetBitForPrefixScan;
    std::vector<long long> durationsPrefixScanAll;
    std::vector<long long> durationsPrefixScanWorkGroupSums;
    std::vector<long long> durationsSortIntermediateData;

    cout << "sorting " << numItemsInPrefixScanBuffer << " items" << endl;

    // moving original data to intermediate data is 1 item per thread
    start = high_resolution_clock::now();
    glUseProgram(_originalDataToIntermediateDataProgramId);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    end = high_resolution_clock::now();
    durationOriginalDataToIntermediateData = duration_cast<microseconds>(end - start).count();
    
    // for 32bit unsigned integers, make 32 passes
    bool readFromFirstIntermediateBuffer = true;
    for (unsigned int bitNumber = 0; bitNumber < 32; bitNumber++)
    {
        unsigned int readBufferNumber = (unsigned int)readFromFirstIntermediateBuffer;

        // getting 1 bit value from intermediate data to prefix sum is 1 item per thread
        start = high_resolution_clock::now();
        glUseProgram(_getBitForPrefixScansProgramId);
        glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readBufferNumber);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        end = high_resolution_clock::now();
        durationsGetBitForPrefixScan.push_back(duration_cast<microseconds>(end - start).count());

        // prefix scan over all values
        // Note: Parallel prefix scan is 2 items per thread.
        start = high_resolution_clock::now();
        glUseProgram(_parallelPrefixScanProgramId);
        glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 1);
        glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        end = high_resolution_clock::now();
        durationsPrefixScanAll.push_back(duration_cast<microseconds>(end - start).count());

        // prefix scan over per-work-group sums
        // Note: The PrefixSumsPerGroup array is sized to be exactly enough for 1 work group.  It 
        // makes the prefix sum easier than trying to eliminate excess threads.
        start = high_resolution_clock::now();
        glUniform1ui(UNIFORM_LOCATION_CALCULATE_ALL, 0);
        glDispatchCompute(1, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        end = high_resolution_clock::now();
        durationsPrefixScanWorkGroupSums.push_back(duration_cast<microseconds>(end - start).count());

        // and sort the intermediate data with the scanned values
        start = high_resolution_clock::now();
        glUseProgram(_sortIntermediateDataProgramId);
        glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, readBufferNumber);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        end = high_resolution_clock::now();
        durationsSortIntermediateData.push_back(duration_cast<microseconds>(end - start).count());

        // now switch intermediate buffers and do it again
        readFromFirstIntermediateBuffer = !readFromFirstIntermediateBuffer;

        printf("-\n");
    }

    // copy the data back and check
    // Note: At the end of the last loop, the "write" buffer became the "read" buffer
    start = high_resolution_clock::now();
    if (readFromFirstIntermediateBuffer)
    {
        // the sorted data wound up in the first buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    }
    else
    {
        // the sorted data wound up in the second buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataSecondBuffer->BufferId());
    }

    std::vector<IntermediateData> checkIntermediateWriteBuffer(numItemsInPrefixScanBuffer);
    unsigned int bufferSizeBytes = checkIntermediateWriteBuffer.size() * sizeof(IntermediateData);
    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkIntermediateWriteBuffer.data(), bufferPtr, bufferSizeBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // check
    for (size_t i = 1; i < checkIntermediateWriteBuffer.size(); i++)
    {
        unsigned int val = checkIntermediateWriteBuffer[i]._data;
        unsigned int prevVal = checkIntermediateWriteBuffer[i - 1]._data;

        if (val == 0xffffffff)
        {
            // this was extra data that was padded on
            continue;
        }

        // the original data is 0 - N-1, 1 value at a time, so it's ok to hard code 
        if (val < prevVal)
        {
            printf("value %u at index %u is >= previous value %u and index %u\n", val, i, prevVal, i - 1);
        }
    }

    end = high_resolution_clock::now();
    durationDataVerification = duration_cast<microseconds>(end - start).count();

    // write the results to stdout and to a text file so that I can dump them into an Excel spreadsheet
    std::ofstream outFile("durations.txt");
    if (outFile.is_open())
    {
        cout << "original data to intermediate data: " << durationOriginalDataToIntermediateData << "\tmicroseconds" << endl;
        outFile << "original data to intermediate data: " << durationOriginalDataToIntermediateData << "\tmicroseconds" << endl;

        cout << "verifying data: " << durationDataVerification << "\tmicroseconds" << endl;
        outFile << "verifying data: " << durationDataVerification << "\tmicroseconds" << endl;

        cout << "getting bits for prefix scan:" << endl;
        outFile << "getting bits for prefix scan:" << endl;
        for (size_t i = 0; i < durationsGetBitForPrefixScan.size(); i++)
        {
            cout << "\t" << i << "\t" << durationsGetBitForPrefixScan[i] << "\tmicroseconds" << endl;
            outFile << "\t" << i << "\t" << durationsGetBitForPrefixScan[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;

        cout << "times for prefix scan over all data:" << endl;
        outFile << "times for prefix scan over all data:" << endl;
        for (size_t i = 0; i < durationsPrefixScanAll.size(); i++)
        {
            cout << "\t" << i << "\t" << durationsPrefixScanAll[i] << "\tmicroseconds" << endl;
            outFile << "\t" << i << "\t" << durationsPrefixScanAll[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;

        cout << "times for prefix scan over work group sums:" << endl;
        outFile << "times for prefix scan over work group sums:" << endl;
        for (size_t i = 0; i < durationsPrefixScanWorkGroupSums.size(); i++)
        {
            cout << "\t" << i << "\t" << durationsPrefixScanWorkGroupSums[i] << "\tmicroseconds" << endl;
            outFile << "\t" << i << "\t" << durationsPrefixScanWorkGroupSums[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;

        cout << "times for sorting intermediate data:" << endl;
        outFile << "times for sorting intermediate data:" << endl;
        for (size_t i = 0; i < durationsSortIntermediateData.size(); i++)
        {
            cout << "\t" << i << "\t" << durationsSortIntermediateData[i] << "\tmicroseconds" << endl;
            outFile << "\t" << i << "\t" << durationsSortIntermediateData[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;
    }
    outFile.close();


    printf("");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

