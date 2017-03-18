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
    typedef std::unique_ptr<PrefixSumSsbo> UNIQUE_PTR;

    void ConfigureUniforms(unsigned int computeProgramId) const override;
    unsigned int NumPerGroupPrefixSums() const;
    unsigned int NumDataEntries() const;

private:
    // NOT the same as the number of items in the buffer
    // Note: The buffer has PerGroupSums and AllPrefixSums.  This item gives the latter's array size.
    unsigned int _numPerGroupPrefixSums;
    unsigned int _numDataEntries;
};
