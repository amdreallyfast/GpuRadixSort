#pragma once

#include "Include/SSBOs/SsboBase.h"
//#include "Shaders/Constants.comp

/*------------------------------------------------------------------------------------------------
Description:
    Encapsulates the SSBO that is used for calculating prefix sums as part of the Z-order curve 
    sorting of particles by position.
Creator:    John Cox, 3/13/2017
------------------------------------------------------------------------------------------------*/
class PrefixSumSsbo : public SsboBase
{
public:
    PrefixSumSsbo(int numParticles);

    void ConfigureCompute(unsigned int computeProgramId, const std::string &bufferNameInShader) override;
    void ConfigureRender(unsigned int renderProgramId, unsigned int drawStyle) override;

};
