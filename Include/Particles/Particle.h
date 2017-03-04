#pragma once

#include "ThirdParty/glm/vec4.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    This is a simple structure that says where a particle is, where it is going, and whether it
    has gone out of bounds ("is active" flag).  That flag also serves to prevent all particles
    from going out all at once upon creation by letting the "particle updater" regulate how many
    are emitted every frame.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
struct Particle
{
    /*-------------------------------------------------------------------------------------------
    Description:
        Sets initial values.  The glm structures have their own zero initialization and I don't 
        care about the integer padding to "is active".  The "is active" flag starts at 0 because 
        I want the first run of the particles to be reset to an emitter.
    Parameters: None
    Returns:    None
    Creator:    John Cox, 10-2-2016
    -------------------------------------------------------------------------------------------*/
    Particle() :
        // glm structures already have "set to 0" constructors
        _collisionCountThisFrame(0),
        _mass(0.1f),
        _radiusOfInfluence(0.01f),
        _indexOfNodeThatItIsOccupying(0),
        _isActive(0)
    {
    }

    // TODO: dd "previous position" too (for collision detection)

    // even though this is a 2D program, I wasn't able to figure out the byte misalignments 
    // between C++ and GLSL (every variable is aligned on a 16byte boundry, but adding 2-float 
    // padding to glm::vec2 didn't work and the compute shader just didn't send any particles 
    // anywhere), so I just used glm::vec4 
    // Note: Yes, I did take care of the byte offset in the vertex attrib pointer.
    glm::vec4 _position;
    glm::vec4 _velocity;
    glm::vec4 _netForceThisFrame;

    // used to determine color 
    // TODO: decide between this for color blending or net force
    int _collisionCountThisFrame;

    // all particles have identical mass for now
    float _mass;

    // used for collision detection because a particle's position is float values, so two
    // particles' position are almost never going to be exactly equal
    float _radiusOfInfluence;

    // will help improve efficiency in particle collision handling
    // Note: Rather than having each node run through its particle collection, which would leave 
    // some shaders with nothing to do and others a lot to do, have the particle collision 
    // calculations go by each particle.  This should even out the load of detecting particle 
    // collisions within a node and withi its neighbors.
    unsigned int _indexOfNodeThatItIsOccupying;

    // Note: Booleans cannot be uploaded to the shader 
    // (https://www.opengl.org/sdk/docs/man/html/glVertexAttribPointer.xhtml), so send the 
    // "is active" flag as an integer.  
    int _isActive; 
    
    // any necessary padding out to 16 bytes to match the GPU's version
    int _padding[3];
};
