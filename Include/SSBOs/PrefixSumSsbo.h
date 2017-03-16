#pragma once

#include "Include/SSBOs/SsboBase.h"
//#include "Shaders/Constants.comp

/*------------------------------------------------------------------------------------------------
Description:
    Encapsulates the SSBO that is used for calculating prefix sums as part of the Z-order curve 
    sorting of particles by position.

    Note: "Prefix scan" vs "prefix sum", same thing.
Creator:    John Cox, 3/13/2017
------------------------------------------------------------------------------------------------*/
class PrefixSumSsbo : public SsboBase
{
public:
    PrefixSumSsbo(int numDataEntries);

    unsigned int NumPerGroupPrefixSums() const;
    unsigned int NumDataEntries() const;

    void ConfigureCompute(unsigned int computeProgramId, const std::string &bufferNameInShader) override;
    void ConfigureRender(unsigned int renderProgramId, unsigned int drawStyle) override;

private:
    // NOT the same as the number of items in the buffer
    // Note: The buffer has perGroupSums and allPrefixSums.  This item gives the latter's array size.
    unsigned int _numPerGroupPrefixSums;
    unsigned int _numDataEntries;
};
