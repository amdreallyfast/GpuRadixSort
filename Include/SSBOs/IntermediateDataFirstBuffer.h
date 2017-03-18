#pragma once

#include "Include/SSBOs/SsboBase.h"


/*------------------------------------------------------------------------------------------------
Description:
    The first of a pair of buffers used during the sorting step of the parallel radix sort.  
    Parallel sorting requires that values go from a position in one buffer to a position in 
    another buffer.  There is no "swap" in parallel.  That might really mess up another threads' 
    juju.

    Intended for use only by the ParallelSort compute controller so that all the buffer sizes 
    are correct.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class IntermediateDataFirstBuffer : public SsboBase
{
public:
    IntermediateDataFirstBuffer(unsigned int numItems);
    typedef std::unique_ptr<IntermediateDataFirstBuffer> UNIQUE_PTR;

    void ConfigureUniforms(unsigned int computeProgramId) const override;
    unsigned int NumItems() const;

private:
    unsigned int _numItems;
};
