#include "Include/SSBOs/PrefixSumSsbo.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"

#include "Shaders/ParallelSort/ParallelSortConstants.comp"
#include "Shaders/ComputeHeaders/SsboBufferBindings.comp"
#include "Shaders/ComputeHeaders/UniformLocations.comp"

#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Further explanation of the number of data entries:
    ----------------------------------------------------------------------------------------------

    This algorithm has each thread working on 2 data items, so the usual uMaxDataCount uniform 
    doesn't have much use as a thread check, but a "max thread count" does.  This value is 
    determined in ParallelSort::Sort().  

    "Max thread count" = num work groups * threads per work group

    Problem: The algorithm relies on a binary tree within a work group's data set, so the number 
    of items that are being summed within a work group must be a power of 2.  Each thread works 
    on 2 items, so the number of threads per work group is also a power of 2 
    (work group size = 256/512/etc.).  We humans like to have our data sets as multiples of 10 
    though, so data set sizes often don't divide evenly by a work group size (256/512/etc.).  

    Solution: Make the variable data set sizes work with the algorithm's reliance on a power of 
    2 by using maximum int values in the excess data.  This will make the radix sort see 1s
    in every bit position and thus sort the excess data to the back.  Then the non-excess data 
    can be accessed using the same indices as the original data.

    Is this wasting threads?  A little.  Only the last work group's data will be padded, and in 
    a large data set (100,000+) there will be dozens of work groups, if not hundreds, so I won't 
    concern myself with trying to optimize that last group's threads.  It is easier to just pad 
    the data and give the threads something to chew on.  Like hay for horses.  It's cheap.

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
    In this example, there are only 42 items worth looking at, so only 21 threads (0 - 20) will 
    be busy doing something useful, but to make the thread count cutoff easier to calculate, the 
    full work group complement of threads will be allowed to run.  

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
    dealing with dummy data that was provided to pad out the data set to the size of a full work 
    group.  This is a pittance compared to the total thread count.
Parameters: 
    numWorkGroups   Num X work groups.  Y and Z are always 1.
Returns:    
    The maximum number of threads allowed in the whole invocation of the ParallelPrefixScan 
    shader.
Creator:    John Cox, 3-16-2017
------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------
Description:
    Initializes the base class, then initializes derived class members and allocates space for 
    the SSBO.
Parameters: 
    numDataEntries  How many items the user wants to have.  The only restriction is that it be 
    less than (due to restrictions in the ParallelPrefixScan) 1024x1024 = 1,048,576.
Returns:    None
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
PrefixSumSsbo::PrefixSumSsbo(unsigned int numDataEntries) :
    SsboBase(),  // generate buffers
    _numPerGroupPrefixSums(0),
    _numDataEntries(0)
{
    // Note: The parallel prefix scan has each work group work on their own data (work group 0 
    // does the prefix sum items 0-1023, work group 1 from 1024-2047, etc.).  It relies of a 
    // binary tree, so the work group size must be a power of 2.  The data that is being scanned 
    // for a prefix sum though might not be a power of 2.  To deal with this, make sure that 
    // there is always a work group for the data to be in (hence the +1), and that entries that 
    // are not part of the data set are set to 0.
    //_numDataEntries = ((numDataEntries / ITEMS_PER_WORK_GROUP) + 1) * ITEMS_PER_WORK_GROUP;
    
    
    // Note: If the user passes in a data set of size 0, then the resultant number of data entries will be 0 and the sorting process will go nowhere.  At least it won't crash.
    // TODO: test this
    if (numDataEntries % ITEMS_PER_WORK_GROUP == 0)
    {
        // data size evenly divides 
        _numDataEntries = numDataEntries / ITEMS_PER_WORK_GROUP;
    }
    else
    {
        // data size does not evenly divide, so give remainder data their own work group
        _numDataEntries = (numDataEntries / ITEMS_PER_WORK_GROUP) + 1;
    }
    _numDataEntries *= ITEMS_PER_WORK_GROUP;

    printf("");

    // Note: The individual work groups' prefix sums are rather useless without the results of 
    // each work group being tallied, so there is one more chunk of data that will store the 
    // total sum from each of the other work groups, and this data will also have the prefix 
    // scan run on it, so it must be the size of one work group's worth of data.  The prefix 
    // scan of the "per work group sums" is a necessary step in preparation for Radix Sort.  If 
    // the number of work groups is greater than the size of one work group's worth of data, 
    // then the prefix sum can't run over the "per work group sums" in one pass.  Force the data 
    // set to be one work group's worth of data to make it easier to handle.  
    _numPerGroupPrefixSums = ITEMS_PER_WORK_GROUP;

    // the std::vector<...>(...) constructor will set everything to 0
    // Note: The +1 is because of a single uint in the buffer, totalNumberOfOnes.  See 
    // explanation in PrefixScanBuffer.comp.
    std::vector<unsigned int> v(_numPerGroupPrefixSums + 1 + _numDataEntries);

    // now bind this new buffer to the dedicated buffer binding location
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PREFIX_SCAN_BUFFER_BINDING, _bufferId);

    // and fill it with 0s
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferId);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size() * sizeof(unsigned int), v.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/*------------------------------------------------------------------------------------------------
Description:
    Defines the buffer's size uniform in the specified shader.  It uses the #define'd uniform 
    location found in UniformLocations.comp.

    If the shader does not have the uniform or if the shader compiler optimized it out, then 
    OpenGL will complain about not finding it.  Enable debugging in main() in main.cpp for more 
    detail.
Parameters: 
    computeProgramId    Self-explanatory.
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/  
void PrefixSumSsbo::ConfigureUniforms(unsigned int computeProgramId) const
{
    // the uniform should remain constant after this 
    glUseProgram(computeProgramId);
    glUniform1ui(UNIFORM_LOCATION_ALL_PREFIX_SUMS_SIZE, _numDataEntries);
    glUseProgram(0);
}

/*------------------------------------------------------------------------------------------------
Description:
    Returns the number of integers that have been allocated for the PrefixSumsPerGroup array.  
    The constructor ensures that this is the size of 1, and only 1, work group's worth of data 
    for the PrefixSumsPerGroup array.
Parameters: None
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
unsigned int PrefixSumSsbo::NumPerGroupPrefixSums() const
{
    return _numPerGroupPrefixSums;
}

/*------------------------------------------------------------------------------------------------
Description:
    Returns the number of integers that have been allocated for the PrefixSumsWithinGroup array.  The 
    constructor ensures that there are enough entries for every item to be part of a work group.  
Parameters: None
Returns:    
    See Description.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
unsigned int PrefixSumSsbo::NumDataEntries() const
{
    return _numDataEntries;
}


