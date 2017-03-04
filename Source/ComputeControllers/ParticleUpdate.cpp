#include "Include/ComputeControllers/ParticleUpdate.h"

#include "Shaders/ShaderStorage.h"
#include "ThirdParty/glload/include/glload/gl_4_4.h"
#include "ThirdParty/glm/gtc/type_ptr.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    Looks up all uniforms in the "particle update" compute shader.

    Note: This constructor takes a string key for the compute shader instead of a program ID 
    because the shader storage object, which is responsible for finding uniforms, takes a shader 
    key instead of a direct program ID.
Parameters: 
    numParticles        Used to tell a shader uniform how big the "all particles" buffer is.
    particleRegionCenter    The region of validity is a circle.  This is the center.
    particleRegionRadius    Self-explanatory in light of the center.
    computeShaderKey    Used to look up (1) the compute shader ID and (2) uniform locations.
Returns:    None
Creator:    John Cox (11-24-2016)
------------------------------------------------------------------------------------------------*/
ComputeParticleUpdate::ComputeParticleUpdate(unsigned int numParticles, 
    const glm::vec4 &particleRegionCenter, const float particleRegionRadius, 
    const std::string &computeShaderKey) :
    _totalParticleCount(0),
    _activeParticleCount(0),
    _computeProgramId(0),
    _acParticleCounterBufferId(0),
    _acParticleCounterCopyBufferId(0),
    _unifLocParticleCount(-1),
    _unifLocParticleRegionCenter(-1),
    _unifLocParticleRegionRadiusSqr(-1),
    _unifLocDeltaTimeSec(-1)
{
    _totalParticleCount = numParticles;

    ShaderStorage &shaderStorageRef = ShaderStorage::GetInstance();

    _unifLocParticleCount = shaderStorageRef.GetUniformLocation(computeShaderKey, "uMaxParticles");
    _unifLocParticleRegionCenter = shaderStorageRef.GetUniformLocation(computeShaderKey, "uParticleRegionCenter");
    _unifLocParticleRegionRadiusSqr = shaderStorageRef.GetUniformLocation(computeShaderKey, "uParticleRegionRadiusSqr");
    _unifLocDeltaTimeSec = shaderStorageRef.GetUniformLocation(computeShaderKey, "uDeltaTimeSec");

    _computeProgramId = shaderStorageRef.GetShaderProgram(computeShaderKey);

    // set uniform values and generate this compute shader's atomic counters
    glUseProgram(_computeProgramId);
    glUniform1ui(_unifLocParticleCount, numParticles);
    glUniform4fv(_unifLocParticleRegionCenter, 1, glm::value_ptr(particleRegionCenter));
    glUniform1f(_unifLocParticleRegionRadiusSqr, particleRegionRadius * particleRegionRadius);
    // delta time set in Update(...)

    // atomic counter initialization courtesy of geeks3D (and my use of glBufferData(...) 
    // instead of glMapBuffer(...)
    // http://www.geeks3d.com/20120309/opengl-4-2-atomic-counter-demo-rendering-order-of-fragments/

    // particle counter
    glGenBuffers(1, &_acParticleCounterBufferId);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _acParticleCounterBufferId);
    GLuint atomicCounterResetVal = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), (void *)&atomicCounterResetVal, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // the atomic counter copy buffer follows suit
    glGenBuffers(1, &_acParticleCounterCopyBufferId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _acParticleCounterCopyBufferId);
    glBufferData(GL_COPY_WRITE_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    // cleanup
    glUseProgram(0);

    // don't need to have a program or bound buffer to set the buffer base
    // Note: It seems that atomic counters must be bound where they are declared and cannot be 
    // bound dynamically like the ParticleSsbo and PolygonSsbo.  So remember to use the SAME buffer 
    // binding base as specified in the shader.
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, _acParticleCounterBufferId);

    // Note: Do NOT bind a buffer base for the "particle counter copy" atomic counter because it 
    // is not used in the "particle update" compute shader.  It is instead meant to copy the 
    // atomic counter buffer before the copy is mapped to a system memory pointer.  Doing this 
    // with the actual atomic counter caused a horrific performance drop.  It appeared to 
    // completely trash the instruction pipeline.
}

/*------------------------------------------------------------------------------------------------
Description:
    Cleans up buffers that were allocated in this object.
Parameters: None
Returns:    None
Creator:    John Cox (10-10-2016)    (created prior to this class in an earlier design)
------------------------------------------------------------------------------------------------*/
ComputeParticleUpdate::~ComputeParticleUpdate()
{
    glDeleteBuffers(1, &_acParticleCounterBufferId);
    glDeleteBuffers(1, &_acParticleCounterCopyBufferId);
}

/*------------------------------------------------------------------------------------------------
Description:
    Resets the atomic counter and dispatches the shader.
    
    The number of work groups is based on the maximum number of particles.
Parameters:    
    deltaTimeSec    Self-explanatory
Returns:    None
Creator:    John Cox (10-10-2016)
            (created in an earlier class, but later split into a dedicated class)
------------------------------------------------------------------------------------------------*/
void ComputeParticleUpdate::Update(const float deltaTimeSec) 
{
    // spread out the particles between lots of work items, but keep it 1-dimensional for easy 
    // navigation through a 1-dimensional particle buffer
    GLuint numWorkGroupsX = (_totalParticleCount / 256) + 1;
    GLuint numWorkGroupsY = 1;
    GLuint numWorkGroupsZ = 1;

    glUseProgram(_computeProgramId);

    glUniform1f(_unifLocDeltaTimeSec, deltaTimeSec);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, _acParticleCounterBufferId);
    unsigned int atomicCounterResetValue = 0;
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), (void *)&atomicCounterResetValue);
    glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    // cleanup
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glUseProgram(0);

    // now that all active particles have updated, check how many active particles exist 
    // Note: Thanks to this post for prompting me to learn about buffer copying to solve this 
    // "extract atomic counter from compute shader" issue.
    // (http://gamedev.stackexchange.com/questions/93726/what-is-the-fastest-way-of-reading-an-atomic-counter) 
    glBindBuffer(GL_COPY_READ_BUFFER, _acParticleCounterBufferId);
    glBindBuffer(GL_COPY_WRITE_BUFFER, _acParticleCounterCopyBufferId);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(GLuint));
    void *bufferPtr = glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
    unsigned int *particleCountPtr = static_cast<unsigned int *>(bufferPtr);
    _activeParticleCount = *particleCountPtr;
    glUnmapBuffer(GL_COPY_WRITE_BUFFER);

    // cleanup
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the number of particles that were active on the last Update(...) call.
    Useful for performance comparison with CPU version.
Parameters: None
Returns:    None
Creator:    John Cox (1-7-2017)
------------------------------------------------------------------------------------------------*/
unsigned int ComputeParticleUpdate::NumActiveParticles() const
{
    return _activeParticleCount;
}
