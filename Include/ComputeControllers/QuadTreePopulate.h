#pragma once

#include <string>
#include "ThirdParty/glm/vec4.hpp"


/*------------------------------------------------------------------------------------------------
Description:
    Controls the compute shader that populates the quad tree with particles.  There is one 
    shader dispatched for every particle.

    That's it.  It was a bit difficult to figure out how to populate a quad tree when 
    (1) There is no malloc/new in a shader.
    (2) It must be thread safe.
    (3) There are no mutexes in a shader.
    (4) Avoid running through the particle array more than once.  
    
    I had a few ideas to deal with these restrictions before concluding that I had no choice but 
    to avoid subdivision.  I tried making my own mutex, but that was extraordinarily slow at 
    best, or crashed FreeGlut, or locked up my computer and forced a hard reset.  I was strongly 
    considering having one shader run for each node, and then each node would then run through 
    the particle collection.  When it was time to subdivide, it would set up its child nodes and 
    then set a flag that would allow the shaders running on its child nodes to begin.  
    Unfortunately, this is both (1) slow because many shaders are waiting on prior ones, and 
    (2) not guaranteed to work because there is no guarantee about which shaders will run first 
    or how many will run at a given time.

    So I concluded: No subdivision in the node.  Pre-subdivide by a large amount, figure out the 
    maximum number of particles that will be put into a node, then increase size of the 
    "max particles per node" value a little beyond that, and let it run.  The only checks are to 
    prevent array overruns.  No subdivision, no reallocation, just excess space.

Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
class ComputeQuadTreePopulate
{
public:
    ComputeQuadTreePopulate(
        unsigned int maxNodes, 
        unsigned int maxParticles, 
        float particleRegionRedius, 
        const glm::vec4 &particleRegionCenter, 
        unsigned int numColumnsInTreeInitial, 
        unsigned int numRowsInTreeInitial, 
        unsigned int numNodesInTreeInitial, 
        const std::string computeShaderKey);
    ~ComputeQuadTreePopulate();

    void PopulateTree();
    unsigned int NumActiveNodes() const;

private:
    unsigned int _computeProgramId;
    unsigned int _totalParticles;
    unsigned int _totalNodes;
    unsigned int _activeNodes;
    unsigned int _initialNodes;

    // all atomic counts are lumped into one buffer
    unsigned int _atomicCounterBufferId;
    unsigned int _acOffsetParticleCounterPerNode;

    // similar to the copy atomic counter in ComputeParticleUpdate for "active particle count", 
    // this is an atomic counter copy buffer for "active node count"
    unsigned int _acNodesInUseCopyBufferId;

    int _unifLocMaxParticles;
    int _unifLocParticleRegionRadius;
    int _unifLocParticleRegionCenter;

    // both the initial number of columns and the initial number of rows are needed for the 
    // "inverse X/Y increment" calculation, but only the initial number of rows needs to stick 
    // around for the PopulateTree() call
    int _unifLocNumColumnsInTreeInitial;
    int _unifLocInverseXIncrementPerColumn;
    int _unifLocInverseYIncrementPerRow;


};