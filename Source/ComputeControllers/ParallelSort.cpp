#include "Include/ComputeControllers/ParallelSort.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Include/SSBOs/PrefixSumSsbo.h"
//#include "Include/SSBOs/IntermediateData.h" // for copying data back and verifying 
#include "Include/SSBOs/OriginalData.h"     // for copying data back and verifying 

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
    This function is used for agressive synchronization.  It is only useful when profiling.
Parameters: None
Returns:    None
Creator:    John Cox, 5/2017
------------------------------------------------------------------------------------------------*/
void WaitForComputeToFinish()
{
    GLsync waitSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glClientWaitSync(waitSync, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
    glDeleteSync(waitSync);
}

/*------------------------------------------------------------------------------------------------
Description:
    Generates multiple compute shaders for the different stages of the parallel sort, and
    allocates various buffers for the sorting.  Buffer sizes are highly dependent on the size of
    the original data.  They are expected to remain constant after class creation.

    The original data's SSBO MUST be passed in so that:
    (1) The uniform specifying buffer size can be set for any compute shaders that use it.
    (2) The sorted OriginalDataCopyBuffer can be copied back to the OriginalDataBuffer.
Parameters:
    dataToSort  See Description.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
ParallelSort::ParallelSort(const OriginalDataSsbo::SHARED_PTR &dataToSort) :
    _originalDataToIntermediateDataProgramId(0),
    _prefixScanStage1ProgramId(0),
    _prefixScanStage2ProgramId(0),
    _prefixScanStage3ProgramId(0),
    _sortIntermediateDataProgramId(0),
    _sortOriginalDataProgramId(0),
    _originalDataCopySsbo(nullptr),
    _intermediateDataSsbo(nullptr),
    _prefixSumSsbo(nullptr),
    _originalDataSsbo(dataToSort)
{
    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();
    std::string shaderKey;


    shaderKey = "prefix scan stage 1";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelPrefixScan1.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _prefixScanStage1ProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    shaderKey = "prefix scan stage 2";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelPrefixScan2.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _prefixScanStage2ProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    shaderKey = "prefix scan stage 3";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelPrefixScan3.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _prefixScanStage3ProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

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

    // and finally sort the "read" array from IntermediateSortBuffers into the "write" array
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

    // after the loop, sort the original data according to the sorted intermediate data
    shaderKey = "sort original data";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/OriginalDataBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/SortOriginalData.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _sortOriginalDataProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    // the size of the OriginalDataBuffer is needed by these shaders, and it is known (as 
    // per my design) only by the OriginalDataSsbo object
    dataToSort->ConfigureConstantUniforms(_originalDataToIntermediateDataProgramId);
    dataToSort->ConfigureConstantUniforms(_sortOriginalDataProgramId);

    unsigned int originalDataSize = dataToSort->NumItems();
    _originalDataCopySsbo = std::make_unique<OriginalDataCopySsbo>(originalDataSize);
    _prefixSumSsbo = std::make_unique<PrefixSumSsbo>(originalDataSize);

    _prefixSumSsbo->ConfigureConstantUniforms(_prefixScanStage1ProgramId);
    _prefixSumSsbo->ConfigureConstantUniforms(_prefixScanStage2ProgramId);
    _prefixSumSsbo->ConfigureConstantUniforms(_prefixScanStage3ProgramId);
    _prefixSumSsbo->ConfigureConstantUniforms(_sortIntermediateDataProgramId);

    _intermediateDataSsbo = std::make_unique<IntermediateDataSsbo>(_prefixSumSsbo->NumDataEntries());
    _intermediateDataSsbo->ConfigureConstantUniforms(_originalDataToIntermediateDataProgramId);
    _intermediateDataSsbo->ConfigureConstantUniforms(_prefixScanStage1ProgramId);
    _intermediateDataSsbo->ConfigureConstantUniforms(_sortIntermediateDataProgramId);

    printf("");
}

/*------------------------------------------------------------------------------------------------
Description:
    This function is the main show of this demo.  It summons shaders to do the following:
    - Copy original data to intermediate data structures 
        Note: If you want to sort your OriginalData structure over a particular value, this is 
        where you decide that.  The rest of the sorting works blindly, bit by bit, on the 
        IntermediateData::_data value.

    - Loop through all 32 bits in an unsigned integer
        - Get bits one at a time from the values in the intermediate data structures
        - Run the parallel prefix scan algorithm on those bit values by work group
        - Run the parallel prefix scan over each work group's sum
        - Sort the IntermediateData structures using the resulting prefix sums
    - Sort the OriginalData items into a copy buffer using the sorted IntermediateData objects
    - Copy the sorted copy buffer back into OriginalDataBuffer

    The OriginalDataBuffer is now sorted.
Parameters: None
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
void ParallelSort::Sort()
{
    // Note: See the explanation at the top of PrefixSumsSsbo.cpp for calculation explanation.
    unsigned int numItemsInPrefixScanBuffer = _prefixSumSsbo->NumDataEntries();

    cout << "sorting " << numItemsInPrefixScanBuffer << " items" << endl;

    // for profiling
    using namespace std::chrono;
    steady_clock::time_point start;
    steady_clock::time_point end;
    long long durationOriginalDataToIntermediateData = 0;
    long long durationDataVerification = 0;
    std::vector<long long> durationsPrefixScan(32);
    std::vector<long long> durationsSortIntermediateData(32);

    // begin
    steady_clock::time_point parallelSortStart = high_resolution_clock::now();

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

    // moving original data to intermediate data is 1 item per thread
    start = high_resolution_clock::now();
    glUseProgram(_originalDataToIntermediateDataProgramId);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    WaitForComputeToFinish();
    end = high_resolution_clock::now();
    durationOriginalDataToIntermediateData = duration_cast<microseconds>(end - start).count();

    // for 32bit unsigned integers, make 32 passes
    bool writeToSecondBuffer = true;
    for (unsigned int bitNumber = 0; bitNumber < 32; bitNumber++)
    {
        // this will either be 0 or half the size of IntermediateDataBuffer
        unsigned int intermediateDataReadBufferOffset = (unsigned int)!writeToSecondBuffer * numItemsInPrefixScanBuffer;
        unsigned int intermediateDataWriteBufferOffset = (unsigned int)writeToSecondBuffer * numItemsInPrefixScanBuffer;

        // prefix sums
        start = high_resolution_clock::now();
        glUseProgram(_prefixScanStage1ProgramId);
        glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_READ_OFFSET, intermediateDataReadBufferOffset);
        glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_WRITE_OFFSET, intermediateDataWriteBufferOffset);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(_prefixScanStage2ProgramId);
        glDispatchCompute(1, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(_prefixScanStage3ProgramId);
        glDispatchCompute(numWorkGroupsXByItemsPerWorkGroup, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        WaitForComputeToFinish();
        end = high_resolution_clock::now();
        durationsPrefixScan[bitNumber] = duration_cast<microseconds>(end - start).count();

        // sort the intermediate data with the prefix sums
        start = high_resolution_clock::now();
        glUseProgram(_sortIntermediateDataProgramId);
        glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_READ_OFFSET, intermediateDataReadBufferOffset);
        glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_WRITE_OFFSET, intermediateDataWriteBufferOffset);
        glUniform1ui(UNIFORM_LOCATION_BIT_NUMBER, bitNumber);
        glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        WaitForComputeToFinish();
        end = high_resolution_clock::now();
        durationsSortIntermediateData[bitNumber] = (duration_cast<microseconds>(end - start).count());

        // now switch intermediate buffers and do it again
        writeToSecondBuffer = !writeToSecondBuffer;
    }

    // now use the sorted IntermediateData objects to sort the original data objects into a copy 
    // buffer (there is no "swap" in parallel sorting, so must write to a dedicated copy buffer
    glUseProgram(_sortOriginalDataProgramId);
    unsigned int intermediateDataReadBufferOffset = (unsigned int)!writeToSecondBuffer * numItemsInPrefixScanBuffer;
    glUniform1ui(UNIFORM_LOCATION_INTERMEDIATE_BUFFER_READ_OFFSET, intermediateDataReadBufferOffset);
    glDispatchCompute(numWorkGroupsXByWorkGroupSize, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // and finally, move the sorted original data from the copy buffer back to the OriginalDataBuffer
    glBindBuffer(GL_COPY_READ_BUFFER, _originalDataCopySsbo->BufferId());
    glBindBuffer(GL_COPY_WRITE_BUFFER, _originalDataSsbo->BufferId());
    unsigned int originalDataBufferSizeBytes = _originalDataSsbo->NumItems() * sizeof(OriginalData);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, originalDataBufferSizeBytes);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    // end sorting
    steady_clock::time_point parallelSortEnd = high_resolution_clock::now();
    WaitForComputeToFinish();

    // verify sorted data
    start = high_resolution_clock::now();
    unsigned int startingIndex = 0;
    std::vector<OriginalData> checkOriginalData(_originalDataSsbo->NumItems());
    unsigned int bufferSizeBytes = checkOriginalData.size() * sizeof(OriginalData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _originalDataSsbo->BufferId());
    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, startingIndex, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkOriginalData.data(), bufferPtr, bufferSizeBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // check
    for (unsigned int i = 1; i < checkOriginalData.size(); i++)
    {
        unsigned int val = checkOriginalData[i]._value;
        unsigned int prevVal = checkOriginalData[i - 1]._value;

        if (val == 0xffffffff)
        {
            // this was extra data that was padded on
            continue;
        }

        // the original data is 0 - N-1, 1 value at a time, so it's ok to hard code 
        if (prevVal > val)
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
        long long totalParallelSortTime = duration_cast<microseconds>(parallelSortEnd - parallelSortStart).count();
        cout << "total sort time: " << totalParallelSortTime << "\tmicroseconds" << endl;
        outFile << "total sort time: " << totalParallelSortTime << "\tmicroseconds" << endl;

        cout << "original data to intermediate data: " << durationOriginalDataToIntermediateData << "\tmicroseconds" << endl;
        outFile << "original data to intermediate data: " << durationOriginalDataToIntermediateData << "\tmicroseconds" << endl;
        
        cout << "verifying data: " << durationDataVerification << "\tmicroseconds" << endl;
        outFile << "verifying data: " << durationDataVerification << "\tmicroseconds" << endl;

        cout << "prefix scan:" << endl;
        outFile << "prefix scan:" << endl;
        for (size_t i = 0; i < durationsPrefixScan.size(); i++)
        {
            cout << i << "\t" << durationsPrefixScan[i] << "\tmicroseconds" << endl;
            outFile << i << "\t" << durationsPrefixScan[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;

        cout << "times for sorting intermediate data:" << endl;
        outFile << "times for sorting intermediate data:" << endl;
        for (size_t i = 0; i < durationsSortIntermediateData.size(); i++)
        {
            cout << i << "\t" << durationsSortIntermediateData[i] << "\tmicroseconds" << endl;
            outFile << i << "\t" << durationsSortIntermediateData[i] << "\tmicroseconds" << endl;
        }
        cout << endl;
        outFile << endl;
    }
    outFile.close();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

