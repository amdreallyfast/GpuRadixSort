#pragma once

#include "Include/SSBOs/SsboBase.h"
#include "Include/Particles/ParticleQuadTreeNode.h"
#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Sets up the Shader Storage Block Object for quad tree nodes.  The polygon will be used in 
    the compute shader only.
Creator:    John Cox, 1/16/2017
------------------------------------------------------------------------------------------------*/
class QuadTreeNodeSsbo : public SsboBase
{
public:
    QuadTreeNodeSsbo(const std::vector<ParticleQuadTreeNode> &nodeCollection);
    virtual ~QuadTreeNodeSsbo();

    void ConfigureCompute(unsigned int computeProgramId, const std::string &bufferNameInShader) override;
    void ConfigureRender(unsigned int renderProgramId, unsigned int drawStyle) override;

private:
    unsigned int _bufferSizeBytes;
};

