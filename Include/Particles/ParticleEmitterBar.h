#pragma once

#include "Include/Particles/IParticleEmitter.h"
#include "Include/Particles/Particle.h"
#include "Include/MinMaxVelocity.h"
#include "ThirdParty/glm/vec2.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    This particle emitter will reset particles to a position along a vector and will set their 
    velocity to the provided vector.  Particles will be launch in this direction evenly 
    distributed along the bar.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
class ParticleEmitterBar : public IParticleEmitter
{
public:
    ParticleEmitterBar(const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &emitDir,
        const float minVel, const float maxVel);
    virtual void SetTransform(const glm::mat4 &emitterTransform) override;

    glm::vec4 GetBarStart() const;
    glm::vec4 GetBarEnd() const;
    glm::vec4 GetEmitDir() const;
    float GetMinVelocity() const;
    float GetDeltaVelocity() const;

private:
    glm::vec4 _start;
    glm::vec4 _end;
    glm::vec4 _emitDir;
    float _minVel;
    float _deltaVelocity;

    glm::vec4 _transformedStart;
    glm::vec4 _transformedEnd;
    glm::vec4 _transformedEmitDir;
};

