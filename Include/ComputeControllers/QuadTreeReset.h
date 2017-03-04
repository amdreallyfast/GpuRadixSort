#pragma once

#include <string>

/*------------------------------------------------------------------------------------------------
Description:
    Encapsulate quad tree resetting via compute shader.  The number of availble nodes is reset 
    to the number of starting nodes (the quad tree is pre-subdivided, so the number of starting 
    nodes is not 0), and the values in each node are set such that they effectively rendered 
    inert.

    Note: No OpenGL objects are generated that need to be destroyed, so there is no destructor.
Creator:    John Cox (1-14-2017)
------------------------------------------------------------------------------------------------*/
class ComputeQuadTreeReset
{
public:
    ComputeQuadTreeReset(unsigned int numStartingNodes, unsigned int maxNodes, 
        const std::string &computeShaderKey);

    void ResetQuadTree();

private:
    unsigned int _computeProgramId;
    unsigned int _totalNodeCount;
    unsigned int _unifLocMaxNodes;
    unsigned int _unifLocNumStartingNodes;

};
