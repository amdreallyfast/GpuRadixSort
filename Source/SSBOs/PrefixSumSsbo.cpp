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
    2 by using maximum int values in the excess data.  This will make the Radix Sort see 1s
    in every bit position and thus sort the excess data to the back.  Then the non-excess data 
    can be accessed using the same indices as the original data.

    Is this wasting threads?  A little.  Only the last work group's data will be padded, and in 
    a large data set (100,000+) there will be dozens of work groups, if not hundreds, so I won't 
    concern myself with trying to optimize that last group's threads.  It is easier to just pad 
    the data and give the threads something to chew on.  Like hay for horses.  It's cheap.

    In the following examples, work group size = 512, so
    ITEMS_PER_WORK_GROUP = work group size * 2 = 1024.

    Ex 1: data size = 42
    allocated data  = ((42 / 1024) + (42 % 1024 == 0) ? 0 : 1) * 1024
                    = (0 + (42 == 0) ? 0 : 1) * 1024
                    = (0 + 1) * 1024
                    = 1 * 1024
                    = 1024
    num work groups = allocated data / 1024
                    = 1
    num threads     = allocated data / 2
                    = 512
    In this example, there are only 42 items worth looking at, so only 21 threads (0 - 20) will 
    be busy doing something useful.  The remaining threads (21 - 511) will be munching on dummy 
    data.  This seems like a waste of threads, but see the later examples.

    Ex 2: data size = 1024
    allocated data  = ((1024 / 1024) + (1024 % 1024 == 0) ? 0 : 1) * 1024
                    = (1 + (0 == 0) ? 0 : 1) * 1024
                    = (1 + 0) * 1024
                    = 1 * 1024
                    = 1024
    num work groups = allocated data / 1024
                    = 1
    number threads  = allocated data / 2
                    = 512
    In this example, the user-provided data size was exactly a multiple of a work group's worth 
    of data.  This is a corner case, but it must be accounted for.

    Ex 3: data size = 2500
    allocated data  = ((2500 / 1024) + (2500 % 1024 == 0) ? 0 : 1) * 1024
                    = (2 + (452 == 0) ? 0 : 1) * 1024
                    = (2 + 1) * 1024
                    = 3 * 1024
                    = 3072
    num work groups = allocated data / 1024
                    = 3
    number threads  = allocated data / 2
                    = 1536
    In this example:
    - 512 threads (0 - 511) calculate the prefix sum for work group 0 (indices 0 - 1023)
    - 512 threads (512 - 1023) calculate the pregix sum for work group 1 (indices 1024 - 2047)
    - 512 threads (1024 - 1535) calculate the prefix sum for work group 2 (indices 2048 - 3072)
    Indices 2500 - 3071 are dummy data that was padded in so that the binary tree algorithm would
    have something for all the threads to do.  This is 571 threads doing something useless, but
    there are 965 threads doing something useful.  Still wasteful, it seems.

    Ex 3: data size = 100,000
    allocated data  = ((100,000 / 1024) + (100,000 % 1024 == 0) ? 0 : 1) * 1024
                    = (97 + (672 == 0) ? 0 : 1) * 1024
                    = (97 + 1) * 1024
                    = 98 * 1024
                    = 100,352
    num work groups = allocated data / 1024
                    = 98
    number threads  = allocated data / 2
                    = 50,176
    In this example there are 352 excess items allocated, which means 176 threads that are 
    dealing with dummy data.  This is a pittance compared to the total thread count, so it 
    doesn't seem wasteful anymore.  

    ParallelPrefixScan does its best work on large data sets (100,000+).
Creator:    John Cox, 3-2017
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
    _numDataEntries(0)
{
    // see explanation essay at the top of the file
    // Note: If the user passes in a data set of size 0, then the following calculation 
    // will give a number of data entries of 0, so the number of work groups calculated in the 
    // ParallelSort compute controller will lso be 0 and the sorting process will go nowhere.  
    // At least it won't crash.
    _numDataEntries = (numDataEntries / ITEMS_PER_WORK_GROUP);
    _numDataEntries += (numDataEntries % ITEMS_PER_WORK_GROUP == 0) ? 0 : 1;
    _numDataEntries *= ITEMS_PER_WORK_GROUP;

    // the std::vector<...>(...) constructor will set everything to 0
    // Note: The +1 is because of a single uint in the buffer, totalNumberOfOnes.  See 
    // explanation in PrefixScanBuffer.comp.
    std::vector<unsigned int> v(1 + _numDataEntries);

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
void PrefixSumSsbo::ConfigureConstantUniforms(unsigned int computeProgramId) const
{
    // the uniform should remain constant after this 
    glUseProgram(computeProgramId);
    glUniform1ui(UNIFORM_LOCATION_ALL_PREFIX_SUMS_MAX_ENTRIES, _numDataEntries);
    glUseProgram(0);
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


