#pragma once

#include "ThirdParty/glm/vec2.hpp"
#include "ThirdParty/glm/vec4.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    Stores all info necessary to draw a single vertex.
Creator:    John Cox (6-12-2016)
------------------------------------------------------------------------------------------------*/
struct MyVertex
{
    /*-------------------------------------------------------------------------------------------
    Description:
        Ensures that the object starts object with initialized values.

        Note: The position argument comes as a vec2 for ease of creation, but it is put into a 
        vec4 for ease of use with the compute shader.  It is given a w=1 so that it is 
        translatable.  The normal is similarly put into a vec4, but it is givecn w=0 so that it 
        is not translateable.
    Parameters:
        pos     The particle's position in window space.  
        normal  Self-explanatory.
    Returns:    None
    Creator:    John Cox, 9-25-2016
    -------------------------------------------------------------------------------------------*/
    MyVertex(const glm::vec2 &pos, const glm::vec2 &normal) :
        _position(pos, 0.0f, 1.0f),
        _normal(normal, 0.0f, 0.0f)
    {

    }

    // even though this is a 2D program, vec4s were chosen because it is easier than trying to 
    // match GLSL's 16-bytes-per-variable with arrays of dummy floats
    glm::vec4 _position;
    glm::vec4 _normal;
};

