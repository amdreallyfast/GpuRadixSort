#pragma once

#include "Include/MyVertex.h"

/*------------------------------------------------------------------------------------------------
Description:
    This is a simple structure that describes the face of a 2D polygon.  The face begins at P1 
    and ends at P2.  In this demo, the normal will be used to calculate collisions.
Creator:    John Cox (9-8-2016)
------------------------------------------------------------------------------------------------*/
struct PolygonFace
{
    /*-------------------------------------------------------------------------------------------
    Description:
        Gives members default values.  This default constructor was created so that a 
        std::vector<...> of them could be created with null values and then shoved into an SSBO 
        for alteration in a compute shader.
    Parameters: None
    Returns:    None
    Creator:    John Cox, 1-16-2017
    -------------------------------------------------------------------------------------------*/
    PolygonFace() :
        _start(glm::vec2(), glm::vec2()),
        _end(glm::vec2(), glm::vec2())
    {
    }

    /*-------------------------------------------------------------------------------------------
    Description:
        Gives members initial values to describe the face of a 2D polygon face.  Without the 
        normals, this would just be a 2D line, but with surface normals the line is technically 
        considered a face.
    Parameters: 
        start   Self-eplanatory.
    Returns:    None
    Creator:    John Cox, 9-25-2016
    -------------------------------------------------------------------------------------------*/
    PolygonFace(const MyVertex &start, const MyVertex &end) :
        _start(start),
        _end(end)
    {
    }

    MyVertex _start;
    MyVertex _end;
};
