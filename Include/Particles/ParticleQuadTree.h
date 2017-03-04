#pragma once

#include <vector>
#include "Include/Particles/ParticleQuadTreeNode.h"
#include "ThirdParty/glm/vec4.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    Starts up the quad tree that will contain the particles.  The constructor initializes the 
    values for all the nodes, in particular calculating each node's edges and neighbors.  After 
    that, it is a dumb structure.  It is meant to be used as setup before uploading the quad 
    tree into an SSBO.
    
    Note: I considered and heavily entertained the idea of starting every frame with one node
    and subdividing as necessary, but I abandoned that idea because there will be many particles
    in all but the initial frames.  It took some extra calculations up front, but the initial
    subdivision should cut down on the subdivisions that are needed on every frame.
Creator:    John Cox (1-10-2017)
------------------------------------------------------------------------------------------------*/
class ParticleQuadTree
{
public:
    ParticleQuadTree(const glm::vec4 &particleRegionCenter, float particleRegionRadius);

    // increase the number of additional nodes as necessary to handle more subdivision
    // Note: This algorithm was built with the compute shader's implementation in mind.  These 
    // structures are meant to be used as if a compute shader was running it, hence all the 
    // arrays and a complete lack of runtime memory reallocation.
    //static const int _NUM_ROWS_IN_TREE_INITIAL = 8;
    //static const int _NUM_COLUMNS_IN_TREE_INITIAL = 8;
    //static const int _NUM_STARTING_NODES = _NUM_ROWS_IN_TREE_INITIAL * _NUM_COLUMNS_IN_TREE_INITIAL;
    static const int _NUM_ROWS_IN_TREE_INITIAL = 64;
    static const int _NUM_COLUMNS_IN_TREE_INITIAL = 64;
    static const int _NUM_STARTING_NODES = _NUM_ROWS_IN_TREE_INITIAL * _NUM_COLUMNS_IN_TREE_INITIAL;

    // if you change this, MUST also change the array size of atomic counters in quadTreePopulate.comp
    static const int _MAX_NODES = _NUM_STARTING_NODES * 8;

    std::vector<ParticleQuadTreeNode> _allQuadTreeNodes;
    glm::vec4 _particleRegionCenter;
    float _particleRegionRadius;

};
