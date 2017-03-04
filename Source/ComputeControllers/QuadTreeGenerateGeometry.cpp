#include "Include/ComputeControllers/QuadTreeGenerateGeometry.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ShaderStorage.h"

/*------------------------------------------------------------------------------------------------
Description:
    Gives members initial values.
    Finds the uniforms for the "generate geometry" compute shader and gives them initial values.
    Generates the atomic counters for this shader.  Values are set in GenerateGeometry().
Parameters:
    maxNodes            Tells the shader how big the "quad tree node" buffer is.
    maxPolygonFaces     Tells the shader how many polygon faces are available.
    computeShaderKey    Used to look up the shader's uniform and program ID.
Returns:    None
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
ComputeQuadTreeGenerateGeometry::ComputeQuadTreeGenerateGeometry(unsigned int maxNodes, 
    unsigned int maxPolygonFaces, const std::string &computeShaderKey) :
    _computeProgramId(0),
    _totalNodes(0),
    _facesInUse(0),
    _atomicCounterBufferId(0),
    _acOffsetPolygonFacesInUse(0),
    _acOffsetPolygonFacesCrudeMutex(0),
    _atomicCounterCopyBufferId(0),
    _unifLocMaxNodes(0),
    _unifLocMaxPolygonFaces(0)
{
    _totalNodes = maxNodes;

    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    _unifLocMaxNodes = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxNodes");
    _unifLocMaxPolygonFaces = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxPolygonFaces");

    _computeProgramId = shaderStorageRef.GetShaderProgram(computeShaderKey);

    glUseProgram(_computeProgramId);

    // set uniform data
    glUniform1ui(_unifLocMaxNodes, maxNodes);
    glUniform1ui(_unifLocMaxPolygonFaces, maxPolygonFaces);

    // atomic counters
    glGenBuffers(1, &_atomicCounterBufferId);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomicCounterBufferId);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint) * 2, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // the atomic counter copy buffer follows suit
    glGenBuffers(1, &_atomicCounterCopyBufferId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _atomicCounterCopyBufferId);
    glBufferData(GL_COPY_WRITE_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    glUseProgram(0);

    // offsets and binding MUST match the offsets specified in the "generate geometry" shader
    _acOffsetPolygonFacesInUse = 0;
    _acOffsetPolygonFacesCrudeMutex = 4;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 4, _atomicCounterBufferId);

    // no binding for the atomic counter copy buffer
}

/*------------------------------------------------------------------------------------------------
Description:
    Deletes the buffer for the atomic counters.
Parameters: None
Returns:    None
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
ComputeQuadTreeGenerateGeometry::~ComputeQuadTreeGenerateGeometry()
{
    glDeleteBuffers(1, &_atomicCounterBufferId);
}

/*------------------------------------------------------------------------------------------------
Description:
    Resets the atomic counters nd dispatches the shader.

    The number of work groups is based on the maximum number of nodes.
Parameters: None
Returns:    None
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
void ComputeQuadTreeGenerateGeometry::GenerateGeometry()
{
    glUseProgram(_computeProgramId);

    // both atomic counters are 0
    // Note: This shader will run through all the nodes and generate new faces for every single 
    // one, so the initial number of faces in use is 0.  

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomicCounterBufferId);

    GLuint zero = 0;
    GLuint sizeOfGlUint = sizeof(GLuint);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, _acOffsetPolygonFacesInUse, sizeOfGlUint, &zero);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, _acOffsetPolygonFacesCrudeMutex, sizeOfGlUint, &zero);

    // calculate the number of work groups and start the magic
    GLuint numWorkGroupsX = (_totalNodes / 256) + 1;
    GLuint numWorkGroupsY = 1;
    GLuint numWorkGroupsZ = 1;

    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // retrieve the number of faces currently in use
    // Note: See ComputeParticleUpdata::Update(...) for more explanation on this.
    glBindBuffer(GL_COPY_READ_BUFFER, _atomicCounterBufferId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _atomicCounterCopyBufferId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, _acOffsetPolygonFacesInUse, 0, sizeof(GLuint));
    void *bufferPtr = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
    unsigned int *faceCountPtr = static_cast<unsigned int *>(bufferPtr);
    _facesInUse = *faceCountPtr;
    glUnmapBuffer(GL_COPY_WRITE_BUFFER);

    // cleanup
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glUseProgram(0);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the number of polygon faces that were in use after the last call to 
    GenerateGeometry().
Parameters: None
Returns:    
    See Description.
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
unsigned int ComputeQuadTreeGenerateGeometry::NumActiveFaces() const
{
    return _facesInUse;
}
