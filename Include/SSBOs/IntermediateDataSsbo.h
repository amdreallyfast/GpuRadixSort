#pragma once

#include "Include/SSBOs/SsboBase.h"

// TODO: rename file -> IntermediataDataSsbo


/*------------------------------------------------------------------------------------------------
Description:
    There is no "swap" in parallel sorting, so this buffer contains enough space for a 
    read/write pair of buffers, each of which is big enough to contain a 
    PrefixScanBuffer::PrefixSumsWithinGroup array's size of info.

    //The first of a pair of buffers used during the sorting step of the parallel radix sort.  
    //Parallel sorting requires that values go from a position in one buffer to a position in 
    //another buffer.  There is no "swap" in parallel.  That might really mess up another threads' 
    //juju.

    Intended for use only by the ParallelSort compute controller so that all "num items" 
    calculations are contained.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class IntermediateDataSsbo : public SsboBase
{
public:
    IntermediateDataSsbo(unsigned int numItems);
    typedef std::unique_ptr<IntermediateDataSsbo> UNIQUE_PTR;

    void ConfigureUniforms(unsigned int computeProgramId) const override;
    unsigned int NumItems() const;

private:
    unsigned int _numItems;
};
