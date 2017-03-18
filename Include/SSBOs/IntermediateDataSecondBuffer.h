#pragma once

#include "Include/SSBOs/SsboBase.h"


/*------------------------------------------------------------------------------------------------
Description:
    The second of a pair of buffers used during the sorting step of the parallel radix sort.  

    Intended for use only by the ParallelSort compute controller so that all the buffer sizes 
    are correct.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class IntermediateDataSecondBuffer : public SsboBase
{
public:
    IntermediateDataSecondBuffer(unsigned int numItems);
    typedef std::unique_ptr<IntermediateDataSecondBuffer> UNIQUE_PTR;

private:
};
