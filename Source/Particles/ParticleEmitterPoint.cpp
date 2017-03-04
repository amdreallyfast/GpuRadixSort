#include "Include/Particles/ParticleEmitterPoint.h"

#include "Include/RandomToast.h"
#include "ThirdParty/glm/detail/func_geometric.hpp" // for normalizing glm vectors

/*------------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters:
    emitterPos  A 2D vector in window space (XY on range [-1,+1]).
    minVel      The minimum velocity for particles being emitted.
    maxVel      The maximum emission velocity.
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
ParticleEmitterPoint::ParticleEmitterPoint(const glm::vec2 &emitterPos, const float minVel, 
    const float maxVel)
{
    // this demo is in window space, so Z pos is 0, but let it be translatable (4th value is 1)
    _pos = glm::vec4(emitterPos, 0.0f, 1.0f);
    _minVel = minVel;
    _deltaVelocity = maxVel - minVel;

    // the transformed variants begin equal to the original points, then diverge after 
    // SetTransform(...) is called
    _transformedPos = _pos;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple gett for the emitter's position.  It is used by ParticlePolygonComputeUpdater.cpp
    to tell the compute shader where this point emitter's base position is.
Parameters: None
Returns:    
    A vec4 that is the emitter's position.
Creator:    John Cox (9-20-2016)
------------------------------------------------------------------------------------------------*/
glm::vec4 ParticleEmitterPoint::GetPos() const
{
    return _transformedPos;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emitter's minimum velocity.  
Parameters: None
Returns:
    A float.
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
float ParticleEmitterPoint::GetMinVelocity() const
{
    return _minVel;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emitter's delta velocity.  
Parameters: None
Returns:
    A float.
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
float ParticleEmitterPoint::GetDeltaVelocity() const
{
    return _deltaVelocity;
}

/*------------------------------------------------------------------------------------------------
Description:
    Why transform this for every emission of every particle when I can do it once before
    particle updating and be done with it for the rest of the frame?
Parameters:
    emitterTransform    Transform emitter's position with this.
Returns:    None
Exception:  Safe
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
void ParticleEmitterPoint::SetTransform(const glm::mat4 &emitterTransform)
{
    _transformedPos = emitterTransform * _pos;
}
