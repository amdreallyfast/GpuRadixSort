#include "Include/SSBOs/PrefixSumSsbo.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ComputeHeaders/ParallelSortConstants.comp"
#include "Shaders/ComputeHeaders/SsboBufferBindings.comp"

#include <vector>

// TODO: header
PrefixSumSsbo::PrefixSumSsbo(int numDataEntries) :
    SsboBase(),  // generate buffers
    _numPerGroupPrefixSums(0),
    _numDataEntries(0)
{
    // see ParallelPrefixScan.comp for buffer details
    // Note: The parallel prefix scan has each work group work on their own data (work group 0 does the prefix sum items 0-1023, work group 1 from 1024-2047, etc.).  It relies of a binary tree, so the work group size must be a power of 2.  The data that is being scanned for a prefix sum though might not be a power of 2.  To deal with this, make sure that there is always a work group for the data to be in, and any data that is not part of the data set will be set to 0.
    unsigned int numWorkGroups = (numDataEntries / ITEMS_PER_WORK_GROUP) + 1;

    // Note: The individual work groups' prefix sums are rather useless without the results of each work group being tallied, so there is one more chunk of data that will store the total sum from each of the other work groups, and this data will also have the prefix scan run on it, so it must be the size of one work group's worth of data.  The prefix scan of the "per work group sums" is a necessary step in preparation for Radix Sort.  If the number of work groups is greater than the size of one work group's worth of data, then the prefix sum can't run over the "per work group sums" in one pass.  Force the data set to be one work group's worth of data to make it easier to handle.  
    _numPerGroupPrefixSums = ITEMS_PER_WORK_GROUP;
    _numDataEntries = numWorkGroups * ITEMS_PER_WORK_GROUP;

    // the std::vector<...>(...) constructor will set everything to 0
    std::vector<unsigned int> v(_numPerGroupPrefixSums + _numDataEntries);

    // no data right now
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size() * sizeof(unsigned int), v.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// TODO: header
// The constructor ensures that this is the size of 1, and only 1, work group's worth of data for the perGroupSums array.
unsigned int PrefixSumSsbo::NumPerGroupPrefixSums() const
{
    return _numPerGroupPrefixSums;
}

// TODO: header
// The constructor ensures that there are enough entries in the allPrefixSums array for every item to be part of a work group.  Extra spaces were padded with 0s.
unsigned int PrefixSumSsbo::NumDataEntries() const
{
    return _numDataEntries;
}

// TODO: header
void PrefixSumSsbo::ConfigureCompute(unsigned int computeProgramId, const std::string &bufferNameInShader)
{
    // TODO: remove this method and replace with hard-coded binding locations from some kind of #define header
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);

    //GLuint storageBlockIndex = glGetProgramResourceIndex(computeProgramId, GL_SHADER_STORAGE_BLOCK, bufferNameInShader.c_str());
    //glShaderStorageBlockBinding(computeProgramId, storageBlockIndex, _ssboBindingPointIndex);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _ssboBindingPointIndex, _bufferId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREFIX_SCAN_BUFFER_BINDING, _bufferId);

    // cleanup
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

// TODO: header
void PrefixSumSsbo::ConfigureRender(unsigned int, unsigned int)
{
    // nothing; compute only
}

