#include "Include/SSBOs/SsboBase.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"

/*------------------------------------------------------------------------------------------------
Description:
    This was created for convenience as I've been making more and more SSBOs.
Parameters: None
Returns:    
    See descriptions.
Creator:    John Cox, 1/2017
------------------------------------------------------------------------------------------------*/
static unsigned int GetNewStorageBlockBindingPointIndex()
{
    static GLuint ssboBindingPointIndex = 0;

    return ssboBindingPointIndex++;
}

/*------------------------------------------------------------------------------------------------
Description:
    Gives members default values.

    Note: As part of the Resource Acquisition is Initialization (RAII) approach that I am trying 
    to use in this program, the SSBOs are generated here.  This means that the OpenGL context 
    MUST be started up prior to initialization.  Prior to OpenGL context instantiation, any 
    gl*(...) function calls will throw an exception.

    The SSBO is linked up with the compute shader in ConfigureCompute(...).
    The VAO is initialized in ConfigureRender(...).

Parameters: None
Returns:    None
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
SsboBase::SsboBase() :
    _vaoId(0),
    _bufferId(0),
    _drawStyle(0),
    _numVertices(0),
    _ssboBindingPointIndex(GetNewStorageBlockBindingPointIndex())
{
    glGenBuffers(1, &_bufferId);
    glGenVertexArrays(1, &_vaoId);
}

/*------------------------------------------------------------------------------------------------
Description:
    Cleans up the buffer and VAO.  If either is 0, then the glDelete*(...) call silently does 
    nothing.
Parameters: None
Returns:    None
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
SsboBase::~SsboBase()
{
    glDeleteBuffers(1, &_bufferId);
    glDeleteVertexArrays(1, &_vaoId);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the vertex array object ID.
Parameters: None
Returns:    
    A copy of the VAO's ID.
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
unsigned int SsboBase::VaoId() const
{
    return _vaoId;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the vertex buffer object's ID.
Parameters: None
Returns:
    A copy of the VBO's ID.
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
unsigned int SsboBase::BufferId() const
{
    return _bufferId;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the draw style for the contained object (GL_TRIANGLES, GL_LINES, 
    GL_POINTS, etc.)
Parameters: None
Returns:
    A copy of the draw style GLenum.
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
unsigned int SsboBase::DrawStyle() const
{
    return _drawStyle;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the number of vertices in the SSBO.  Used in the glDrawArrays(...) call.
Parameters: None
Returns:
    See description.
Creator:    John Cox, 9-20-2016
------------------------------------------------------------------------------------------------*/
unsigned int SsboBase::NumVertices() const
{
    return _numVertices;
}

///*------------------------------------------------------------------------------------------------
//Description:
//    At this point in my demos, I have three SSBO structures: particle, polygon face, and quad tree node.  The polygon face SSBO may end up being used more than once, so there is at least 3, possibly more, SSBOs that need unique binding points.  
//    
//    
//    I can't do a simple increment though, because the SSBO needs to keep its binding point index across 
//    
//
//
//Parameters: None
//Returns:
//    See description.
//Creator:    John Cox, 9-20-2016
//------------------------------------------------------------------------------------------------*/
//unsigned int SsboBase::GetStorageBlockBindingPointIndexForBuffer(const std::string &bufferNameInShader)
//{
//    static GLuint ssboBindingPointIndex = 0;
//
//    return ssboBindingPointIndex++;
//}

