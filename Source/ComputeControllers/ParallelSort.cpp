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
    Generates multiple compute shaders for the different stages of the parallel sort.
Parameters: 
    originalDataSize    Self-explanatory.  This should never change after object creation.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
ParallelSort::ParallelSort(unsigned int originalDataSize) :
    _originalDataToIntermediateDataProgramId(0),
    _getBitForPrefixSumsProgramId(0),
    _parallelPrefixScanProgramId(0),
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
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediatSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/OriginalDataToIntermediateData.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _originalDataToIntermediateDataProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    // on each loop in Sort(), pluck out a single bit and add it to the 
    // PrefixScanBuffer::AllPrefixSums array
    shaderKey = "get bit for prefix sums";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/IntermediateSortBuffers.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/GetBitForPrefixSums.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _getBitForPrefixSumsProgramId = shaderStorageRef.GetShaderProgram(shaderKey);

    // run the prefix scan over PrefixScanBuffer::AllPrefixSums, and after run the scan again 
    // over PrefixScanBuffer::PerGroupSums
    shaderKey = "parallel prefix scan";
    shaderStorageRef.NewCompositeShader(shaderKey);
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/Version.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/UniformLocations.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ComputeHeaders/SsboBufferBindings.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/ParallelSortConstants.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParallelSort/PrefixScanBuffer.comp");
    shaderStorageRef.AddPartialShaderFile(shaderKey, "Shaders/ParalleltSort/ParallelPrefixScan.comp");
    shaderStorageRef.CompileCompositeShader(shaderKey, GL_COMPUTE_SHADER);
    shaderStorageRef.LinkShader(shaderKey);
    _parallelPrefixScanProgramId = shaderStorageRef.GetShaderProgram(shaderKey);


    _originalDataCopySsbo = std::make_unique<OriginalDataCopySsbo>(originalDataSize);
    _prefixSumSsbo = std::make_unique<PrefixSumSsbo>(originalDataSize);

    // see explanation in the PrefixSumSsbo constructor for why there are likely more entires in 
    // PrefixScanBuffer::AllPrefixSums than the requested number of items that need sorting
    unsigned int numEntriesInPrefixSumBuffer = _prefixSumSsbo->NumDataEntries();
    _intermediateDataFirstBuffer = std::make_unique<IntermediateDataFirstBuffer>(numEntriesInPrefixSumBuffer);
    _intermediateDataSecondBuffer = std::make_unique<IntermediateDataSecondBuffer>(numEntriesInPrefixSumBuffer);

}

// TODO: header
void ParallelSort::Sort()
{
    // TODO: (if actually sorting original data) need an SsboBase pointer and need to copy the original data buffer to the original data copy buffer

    int numWorkGroupsX = 1;
    int numWorkGroupsY = 1;
    int numWorkGroupsZ = 1;

    using namespace std::chrono;
    // give the counters initial data
    steady_clock::time_point start;
    steady_clock::time_point end;

    // for debugging (TODO: remove this)
    unsigned int numItemsInPrefixScanBuffer = _prefixSumSsbo->NumDataEntries();

    start = high_resolution_clock::now();
    glUseProgram(_originalDataToIntermediateDataProgramId);

    // Note: +1 to total so that a data set less than a single work group size still gets a work 
    // group. 
    // Also Note: +1 to work group size so that a data set size that is an exact multiple of the 
    // work group size doesn't get an extra work group.
    numWorkGroupsX = (numItemsInPrefixScanBuffer / (PARALLEL_SORT_WORK_GROUP_SIZE_X + 1)) + 1;

    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);


    glUniform1ui(UNIFORM_LOCATION_READ_FROM_FIRST_INTERMEDIATE_BUFFER, 1);

    // copy the data back and check
    std::vector<IntermediateData> checkIntermediateData(numItemsInPrefixScanBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _intermediateDataFirstBuffer->BufferId());
    unsigned int bufferSizeBytes = checkIntermediateData.size() * sizeof(IntermediateData);
    void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    memcpy(checkIntermediateData.data(), bufferPtr, bufferSizeBytes);

    printf("");


    //// TODO: replace 512 with a constant
    ////int threadsPerWorkGroup = 512;
    ////int dataSizePerWorkGroup = threadsPerWorkGroup * 2;
    ////int intArrByteOffset = dataSizePerWorkGroup * sizeof(unsigned int);
    //int dataSize = _prefixSumSsbo->NumDataEntries();
    //int PerGroupSumsDataSize = _prefixSumSsbo->NumPerGroupPrefixSums();
    //int totalItems = PerGroupSumsDataSize + dataSize;
    //std::vector<unsigned int> dataToSum;
    ////for (int i = 0; i < numItems; i++)
    //for (int i = 0; i < dataSize; i++)
    //{
    //    // this will push back an alternating assortment of 0s and 1s
    //    dataToSum.push_back(i % 2);
    //}
    //std::random_shuffle(dataToSum.begin(), dataToSum.end());

    //end = high_resolution_clock::now();
    //cout << "generating " << dataSize << " numbers: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    //start = high_resolution_clock::now();
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _prefixSumSsbo->BufferId());
    //unsigned int offsetBytes = PerGroupSumsDataSize * sizeof(unsigned int);
    //unsigned int dataSizeBytes = dataToSum.size() * sizeof(unsigned int);
    //glBufferSubData(GL_SHADER_STORAGE_BUFFER, offsetBytes, dataSizeBytes, dataToSum.data());
    //end = high_resolution_clock::now();
    //cout << "copying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    //// TODO: replace hard-coded 512 with a constant from a header/compute-kosher include file
    //// Note: Divide by "data size per work group" + 1 because, if the number of items == "data size per work group", then numWorkGroupsX = (X/X) + 1 = 2.  This is a corner case, but it needs to be accounted for.
    //int numWorkGroupsX = (dataSize / (ITEMS_PER_WORK_GROUP + 1)) + 1;
    //int numWorkGroupsY = 1;
    //int numWorkGroupsZ = 1;

    //start = high_resolution_clock::now();

    //// calculate all the prefix sums and fill out each work group's PerGroupSums value
    ////glUniform1ui(_unifLocMaxThreadCount, CalculateMaxThreadCount(numWorkGroupsX));
    //glUniform1ui(_unifLocCalculateAll, 1);    
    //glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);

    //// now run the prefix scan over the PerGroupSums array
    //// Note: The PerGroupSums array is only the size of 1 work group, so hard-code 1s.
    ////glUniform1ui(_unifLocMaxThreadCount, PARALLEL_SORT_WORK_GROUP_SIZE_X);
    //glUniform1ui(_unifLocCalculateAll, 0);
    //glDispatchCompute(1, 1, 1);
    // 

    ////glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    //end = high_resolution_clock::now();
    //cout << "running prefix scan with " << (dataSize >> 1) << " threads over " << numWorkGroupsX << " work groups: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;


    ////int totalItems = dataSizePerWorkGroup + dataToSum.size();
    //int bufferSizeBytes = totalItems * sizeof(unsigned int);

    //start = high_resolution_clock::now();
    //void *bufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeBytes, GL_MAP_READ_BIT);
    //std::vector<unsigned int> v(totalItems);
    //memcpy(v.data(), bufferPtr, bufferSizeBytes);
    //end = high_resolution_clock::now();
    //cout << "copying data back: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    //// check the GPU's results
    //start = high_resolution_clock::now();
    //unsigned int manualPrefixSum = 0;
    //for (unsigned int prefixSumIndex = 1; prefixSumIndex < dataToSum.size(); prefixSumIndex++)
    //{
    //    // remember to account for the "per work group sums" that comes before the individual prefix scans
    //    unsigned int currentPrefixSum = v[ITEMS_PER_WORK_GROUP + prefixSumIndex];

    //    if (prefixSumIndex % ITEMS_PER_WORK_GROUP == 0)
    //    {
    //        // reset
    //        manualPrefixSum = 0;
    //    }
    //    else
    //    {
    //        manualPrefixSum += dataToSum[prefixSumIndex - 1];
    //    }

    //    if (currentPrefixSum != manualPrefixSum)
    //    {
    //        printf("");
    //    }
    //}

    //end = high_resolution_clock::now();
    //cout << "verifying data: " << duration_cast<microseconds>(end - start).count() << " microseconds" << endl;

    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

}

