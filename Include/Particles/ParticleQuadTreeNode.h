#pragma once

#include <memory>

/*------------------------------------------------------------------------------------------------
Description:
    Contains all info necessary for a single node of the quad tree.  It is a dumb container 
    meant for use only by ParticleQuadTree.
Creator:    John Cox (12-17-2016)
------------------------------------------------------------------------------------------------*/
struct ParticleQuadTreeNode
{
    ParticleQuadTreeNode() :
        _numCurrentParticles(0),
        _inUse(0),
        _isSubdivided(0),
        _childNodeIndexTopLeft(0),
        _childNodeIndexTopRight(0),
        _childNodeIndexBottomRight(0),
        _childNodeIndexBottomLeft(0),
        _leftEdge(0.0f),
        _topEdge(0.0f),
        _rightEdge(0.0f),
        _bottomEdge(0.0f),
        _neighborIndexLeft(0),
        _neighborIndexTopLeft(0),
        _neighborIndexTop(0),
        _neighborIndexTopRight(0),
        _neighborIndexRight(0),
        _neighborIndexBottomRight(0),
        _neighborIndexBottom(0),
        _neighborIndexBottomLeft(0)
    {
        memset(_indicesForContainedParticles, 0, sizeof(int) * MAX_PARTICLES_PER_QUAD_TREE_NODE);
    }

    static const unsigned int MAX_PARTICLES_PER_QUAD_TREE_NODE = 100;

    unsigned int _indicesForContainedParticles[MAX_PARTICLES_PER_QUAD_TREE_NODE];
    unsigned int _numCurrentParticles;

    int _inUse;
    int _isSubdivided;

    // changed from "int" in the CPU version to "unsigned int" in the GPU version because atomic 
    // counters can only be unsigned integers and the compiler will throw an error rather than 
    // cast a signed to an unsigned
    unsigned int _childNodeIndexTopLeft;
    unsigned int _childNodeIndexTopRight;
    unsigned int _childNodeIndexBottomRight;
    unsigned int _childNodeIndexBottomLeft;

    // left and right edges implicitly X, top and bottom implicitly Y
    float _leftEdge;
    float _topEdge;
    float _rightEdge;
    float _bottomEdge;

    // while not technically necessary because the indices are generated on the CPU side and do 
    // not touch an atomic counter in any compute shader, I changed them to unsigned to match 
    // the child node indices (standardizing my indices)
    unsigned int _neighborIndexLeft;
    unsigned int _neighborIndexTopLeft;
    unsigned int _neighborIndexTop;
    unsigned int _neighborIndexTopRight;
    unsigned int _neighborIndexRight;
    unsigned int _neighborIndexBottomRight;
    unsigned int _neighborIndexBottom;
    unsigned int _neighborIndexBottomLeft;

};
