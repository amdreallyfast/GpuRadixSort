#pragma once

#include "Include/SSBOs/SsboBase.h"

/*------------------------------------------------------------------------------------------------
Description:
    Encapsulates the SSBO that is used for calculating prefix sums as part of the parallel radix 
    sorting algorithm.

    Note: "Prefix scan", "prefix sum", same thing.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class PrefixSumSsbo : public SsboBase
{
public:
    PrefixSumSsbo(unsigned int numDataEntries);
    typedef std::shared_ptr<PrefixSumSsbo> SHARED_PTR;

    void ConfigureConstantUniforms(unsigned int computeProgramId) const override;
    unsigned int NumDataEntries() const;

private:
    unsigned int _numDataEntries;
};
