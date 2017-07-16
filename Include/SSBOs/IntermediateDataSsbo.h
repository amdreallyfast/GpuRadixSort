#pragma once

#include "Include/SSBOs/SsboBase.h"

/*------------------------------------------------------------------------------------------------
Description:
    There is no "swap" in parallel sorting, so this buffer contains enough space for a 
    read/write pair of buffers, each of which is big enough to contain a 
    PrefixScanBuffer::AllPrefixSums array's size of info.

    Intended for use only by the ParallelSort compute controller so that all "num items" 
    calculations are contained.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class IntermediateDataSsbo : public SsboBase
{
public:
    IntermediateDataSsbo(unsigned int numItems);
    typedef std::shared_ptr<IntermediateDataSsbo> SHARED_PTR;

    void ConfigureConstantUniforms(unsigned int computeProgramId) const override;
    unsigned int NumItems() const;

private:
    unsigned int _numItems;
};
