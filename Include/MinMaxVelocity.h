#pragma once

#include "ThirdParty/glm/vec2.hpp"
#include "ThirdParty/glm/vec4.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    Each particle emitter needs be able to generate a velocity direction and magnitude within
    certain constraints.  This class encapsulates these requirements.

    Setters are used instead of initialization upon creation because the particle emitters don't
    have all the information to use this class upon their creation and will need to initialize 
    this class post creation.
Creator:    John Cox (7-4-2016)
------------------------------------------------------------------------------------------------*/
class MinMaxVelocity
{
public:
    MinMaxVelocity();

    void SetMinMaxVelocity(const float min, const float max);
    void SetDir(const glm::vec2 &dir);
    void UseRandomDir();

    glm::vec4 GetNew() const;
    float GetMinVelocity() const;
    float GetDeltaVelocity() const;

private:
    // why store the max if I'm going to be calculating the delta all the time?
    float _velocityDelta;
    float _min;
    bool _useRandomDir;
    glm::vec4 _dir;
};