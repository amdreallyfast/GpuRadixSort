#pragma once

#include "Include/Particles/IParticleEmitter.h"
#include "Include/Particles/ParticleEmitterPoint.h"
#include "Include/Particles/ParticleEmitterBar.h"
#include <string>
#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Encapsulate particle reseting via compute shader.  Resetting involves taking inactive 
    particles and giving them a new position near a particle emitter plus giving them a new 
    velocity.  There is one shader that is dedicated to the task of resetting particles, and 
    this shader is set up to summon and communicate with that particular shader.

    Note: This class is not concerned with the particle SSBO.  It is concerned with uniforms and 
    summoning the shader.  SSBO setup is performed in the appropriate SSBO object.

    Note: When this class goes "poof", it won't delete the emitter pointers.  This is ensured by
    only using const pointers.
Creator:    John Cox (11-24-2016)
------------------------------------------------------------------------------------------------*/
class ComputeParticleReset
{
public:
    ComputeParticleReset(unsigned int numParticles, const std::string &computeShaderKey);
    ~ComputeParticleReset();

    bool AddEmitter(const IParticleEmitter *pEmitter);

    void ResetParticles(unsigned int particlesPerEmitterPerFrame);

private:
    unsigned int _totalParticleCount;
    unsigned int _computeProgramId;

    //unsigned int _acParticleCounterBufferId;
    //unsigned int _acRandSeedBufferId;

    // the atomic counters are both in a single buffer
    unsigned int _atomicCounterBufferId;

    // this the atomic counter is used to enforce the number of emitted particles per emitter 
    // per frame 
    unsigned int _acParticleCounterOffset;

    // another atomic counter is used as a seed for the random hash
    // Note: Rather than needing to seed the position and velocity as I did in the 
    // "single emitter" project, use an atomic counter.  If it reaches maximum unsigned int, 
    // that is ok.  The value will wrap around to 0 and begin again.  
    unsigned int _acRandSeedOffset;

    // unlike most OpenGL IDs, uniform locations are GLint
    int _unifLocParticleCount;
    int _unifLocMaxParticleEmitCount;
    int _unifLocMinParticleVelocity;
    int _unifLocDeltaParticleVelocity;
    int _unifLocUsePointEmitter;
    int _unifLocPointEmitterCenter;
    int _unifLocBarEmitterP1;
    int _unifLocBarEmitterP2;
    int _unifLocBarEmitterEmitDir;

    // all the updating heavy lifting goes on in the compute shader, so CPU cache coherency is 
    // not a concern for emitter storage on the CPU side and a std::vector<...> is acceptable
    // Note: The compute shader has no concept of inheritance.  Rather than store a single 
    // collection of IParticleEmitter pointers and cast them to either point or bar emitters on 
    // every update, just store them separately.
    static const int MAX_EMITTERS = 4;
    std::vector<const ParticleEmitterPoint *> _pointEmitters;
    std::vector<const ParticleEmitterBar *> _barEmitters;
};
