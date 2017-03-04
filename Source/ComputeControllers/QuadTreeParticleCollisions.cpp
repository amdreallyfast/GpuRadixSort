#include "Include/ComputeControllers/QuadTreeParticleCollisions.h"

#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "Shaders/ShaderStorage.h"


/*------------------------------------------------------------------------------------------------
Description:
    Gives members initial values.
    Finds the uniforms for the "particle collisions" compute shader and gives them initial values.
Parameters:
    maxParticles            Tells the shader how big the "particle" buffer is.
    computeShaderKey        Used to look up the shader's uniform and program ID.
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
ComputeParticleQuadTreeCollisions::ComputeParticleQuadTreeCollisions(unsigned int maxParticles, const std::string computeShaderKey) :
    _computeProgramId(0),
    _totalParticles(0),
    _unifLocMaxParticles(-1),
    _unifLocInverseDeltaTimeSec(-1)
{
    _totalParticles = maxParticles;

    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    _unifLocMaxParticles = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxParticles");
    _unifLocInverseDeltaTimeSec = shaderStorageRef.GetUniformLocation(computeShaderKey, "uInverseDeltaTimeSec");

    _computeProgramId = shaderStorageRef.GetShaderProgram(computeShaderKey);

    glUseProgram(_computeProgramId);

    // uniform initialization
    glUniform1ui(_unifLocMaxParticles, maxParticles);

    // the "inverse delta time" uniform will be uploaded in Update(...)

    glUseProgram(0);
}

/*------------------------------------------------------------------------------------------------
Description:
    Dispatches the shader.  

    The number of work groups is based on the maximum number of particles.
Parameters: None
Returns:    None
Creator:    John Cox (1-21-2017)
------------------------------------------------------------------------------------------------*/
void ComputeParticleQuadTreeCollisions::Update(float deltaTimeSec)
{
    // calculate the number of work groups and start the magic
    GLuint numWorkGroupsX = (_totalParticles / 256) + 1;
    GLuint numWorkGroupsY = 1;
    GLuint numWorkGroupsZ = 1;

    glUseProgram(_computeProgramId);

    // because doing the division once will be faster than doing the division once per particle 
    // per collision
    // Note: At this time (1-21-2016), the demo maxes out at ~19,000 particles active at one 
    // time.  Each of those particles could collide with 100 (max particles per node is 100 
    // right now) other particles within a node or with more if it is near a node's edge.  
    // Granted having that many particles in such a small space would be quite unusual, but my 
    // point is that there are a lot of collision calculations, part of which requires dividing 
    // by delta time.  So I'll do the division once and then let the compute shader rest easy 
    // with a multiply.
    float inverseDeltaTime = 1.0f / deltaTimeSec;
    glUniform1f(_unifLocInverseDeltaTimeSec, inverseDeltaTime);

    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glUseProgram(0);

}


