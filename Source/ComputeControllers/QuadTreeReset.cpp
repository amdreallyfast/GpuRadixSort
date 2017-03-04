#include "Include/ComputeControllers/QuadTreeReset.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ShaderStorage.h"


/*------------------------------------------------------------------------------------------------
Description:
    Gives members initial values.
    Finds the uniforms for the "generate geometry" compute shader and gives them initial values.
Parameters:
    numStartingNodes    If a node was not in the group that was calculated on initialization, it 
        is disabled.
    maxNodes            Tells the shader how big the "quad tree node" buffer is.
    computeShaderKey    Used to look up the shader's uniform and program ID.
Returns:    None
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
ComputeQuadTreeReset::ComputeQuadTreeReset(unsigned int numStartingNodes, unsigned int maxNodes, 
    const std::string &computeShaderKey)
{
    _totalNodeCount = maxNodes;
    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    _unifLocNumStartingNodes = shaderStorageRef.GetUniformLocation(computeShaderKey, "uNumStartingNodes");
    _unifLocMaxNodes = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxNodes");

    _computeProgramId = shaderStorageRef.GetShaderProgram(computeShaderKey);

    // upload uniform values
    glUseProgram(_computeProgramId);
    glUniform1ui(_unifLocNumStartingNodes, numStartingNodes);
    glUniform1ui(_unifLocMaxNodes, maxNodes);
    glUseProgram(0);
}

/*------------------------------------------------------------------------------------------------
Description:
    Dispatches the shader.

    The number of work groups is calculated according to the maximum number of nodes.
Parameters: None
Returns:    None
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
void ComputeQuadTreeReset::ResetQuadTree()
{
    GLuint numWorkGroupsX = (_totalNodeCount / 256) + 1;
    GLuint numWorkGroupsY = 1;
    GLuint numWorkGroupsZ = 1;

    glUseProgram(_computeProgramId);

    // compute ALL the resets!
    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);

    //??is the vertex attrib array barrier bit necessary after a reset, before there are any new values??
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    glUseProgram(0);
}
