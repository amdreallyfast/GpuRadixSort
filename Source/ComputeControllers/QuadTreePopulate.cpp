#include "Include/ComputeControllers/QuadTreePopulate.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "ThirdParty/glm/gtc/type_ptr.hpp"
#include "Shaders/ShaderStorage.h"

#include <vector>


/*------------------------------------------------------------------------------------------------
Description:
    Gives members initial values.
    Finds the uniforms for the "populate quad tree" compute shader and gives them initial values.
    Generates the atomic counters for this shader.  Values are set in PopulateTree().
Parameters:
    maxNodes                Tells the shader how big the "quad tree node" buffer is.
    maxParticles            Tells the shader how big the "particle" buffer is.
    particleRegionRadius    Used when a particle is calculating its containing node.
    particleRegionCenter    Ditto
    numColumnsInTreeInitial Ditto
    numRowsInTreeInitial    Ditto
    numNodesInTreeInitial   Ditto
    computeShaderKey        Used to look up the shader's uniform and program ID.
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
ComputeQuadTreePopulate::ComputeQuadTreePopulate(
    unsigned int maxNodes,
    unsigned int maxParticles,
    float particleRegionRedius,
    const glm::vec4 &particleRegionCenter,
    unsigned int numColumnsInTreeInitial,
    unsigned int numRowsInTreeInitial,
    unsigned int numNodesInTreeInitial,
    const std::string computeShaderKey) :
    _computeProgramId(0),
    _totalParticles(0),
    _totalNodes(0),
    _activeNodes(0),
    _initialNodes(0),
    _atomicCounterBufferId(0),
    _acOffsetParticleCounterPerNode(0),
    _acNodesInUseCopyBufferId(0),
    _unifLocMaxParticles(-1),
    _unifLocParticleRegionRadius(-1),
    _unifLocParticleRegionCenter(-1),
    _unifLocNumColumnsInTreeInitial(-1),
    _unifLocInverseXIncrementPerColumn(-1),
    _unifLocInverseYIncrementPerRow(-1)
{
    _totalParticles = maxParticles;
    _totalNodes = maxNodes;
    _initialNodes = numNodesInTreeInitial;

    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    _unifLocMaxParticles = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxParticles");
    _unifLocParticleRegionRadius = shaderStorageRef.GetUniformLocation(computeShaderKey, "uParticleRegionRadius");
    _unifLocParticleRegionCenter = shaderStorageRef.GetUniformLocation(computeShaderKey, "uParticleRegionCenter");
    _unifLocNumColumnsInTreeInitial = shaderStorageRef.GetUniformLocation(computeShaderKey, "uNumColumnsInTreeInitial");
    _unifLocInverseXIncrementPerColumn = shaderStorageRef.GetUniformLocation(computeShaderKey, "uInverseXIncrementPerColumn");
    _unifLocInverseYIncrementPerRow = shaderStorageRef.GetUniformLocation(computeShaderKey, "uInverseYIncrementPerRow");

    _computeProgramId = shaderStorageRef.GetShaderProgram(computeShaderKey);

    glUseProgram(_computeProgramId);

    // uniform initialization
    // Note: All of these are constant throughout the program.
    glUniform1ui(_unifLocMaxParticles, maxParticles);
    glUniform1f(_unifLocParticleRegionRadius, particleRegionRedius);
    glUniform4fv(_unifLocParticleRegionCenter, 1, glm::value_ptr(particleRegionCenter));
    glUniform1ui(_unifLocNumColumnsInTreeInitial, numColumnsInTreeInitial);

    float xIncrementPerColumn = 2.0f * particleRegionRedius / numColumnsInTreeInitial;
    float yIncrementPerRow = 2.0f * particleRegionRedius / numRowsInTreeInitial;
    float inverseXIncrementPerColumn = 1.0f / xIncrementPerColumn;
    float inverseYIncrementPerRow = 1.0f / yIncrementPerRow;
    glUniform1f(_unifLocInverseXIncrementPerColumn, inverseXIncrementPerColumn);
    glUniform1f(_unifLocInverseYIncrementPerRow, inverseYIncrementPerRow);

    // atomic counters (1 per node)
    glGenBuffers(1, &_atomicCounterBufferId);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomicCounterBufferId);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint) * maxNodes, 0, GL_DYNAMIC_DRAW);

    // now the copy buffer
    glGenBuffers(1, &_acNodesInUseCopyBufferId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _acNodesInUseCopyBufferId);
    glBufferData(GL_COPY_WRITE_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    // cleanup
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glUseProgram(0);

    // don't need to have a program or bound buffer to set the buffer base
    // Note: The binding and the offsets MUST match those in the "quad tree populate" compute shader.
    _acOffsetParticleCounterPerNode = 0;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, _atomicCounterBufferId);

    // no base binding for the atomic counter copy buffer because that is not used in the shader
}

/*------------------------------------------------------------------------------------------------
Description:
    Deletes the atomic counter buffer.
Parameters: None
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
ComputeQuadTreePopulate::~ComputeQuadTreePopulate()
{
    glDeleteBuffers(1, &_atomicCounterBufferId);
}

/*------------------------------------------------------------------------------------------------
Description:
    Resets the atomic counters and dispatches the shader.

    The number of work groups is based on the maximum number of particles.
Parameters: None
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
void ComputeQuadTreePopulate::PopulateTree()
{
    // reset atomic counters
    // Note: No program biding is required to bind and set the values of the atomic counters.
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _atomicCounterBufferId);
    static std::vector<GLuint> allZeros(_totalNodes, 0);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, _acOffsetParticleCounterPerNode, sizeof(GLuint) * _totalNodes, allZeros.data());
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // calculate the number of work groups and start the magic
    GLuint numWorkGroupsX = (_totalParticles / 256) + 1;
    GLuint numWorkGroupsY = 1;
    GLuint numWorkGroupsZ = 1;
    glUseProgram(_computeProgramId);
    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glUseProgram(0);


    // at this time, the number of nodes is constant
    _activeNodes = _totalNodes;


    //// retrieve the number of active nodes in use (printed to screen)
    //glBindBuffer(GL_COPY_READ_BUFFER, _atomicCounterBufferId);
    //glBindBuffer(GL_COPY_WRITE_BUFFER, _acNodesInUseCopyBufferId);
    //glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, _acOffsetNodesInUse, 0, sizeof(GLuint));
    //void *bufferPtr = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
    //unsigned int *nodeCountPtr = static_cast<unsigned int *>(bufferPtr);
    //_activeNodes = *nodeCountPtr;
    //glUnmapBuffer(GL_COPY_WRITE_BUFFER);
    //glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    //glBindBuffer(GL_COPY_READ_BUFFER, 0);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the number of active nodes.  Used during drawing to report the number of 
    nodes in use.
Parameters: None
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
unsigned int ComputeQuadTreePopulate::NumActiveNodes() const
{
    return _activeNodes;
}




