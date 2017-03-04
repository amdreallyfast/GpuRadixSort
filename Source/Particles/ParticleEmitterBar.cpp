#include "Include/Particles/ParticleEmitterBar.h"

#include "Include/RandomToast.h"

/*------------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters:
    p1      The bar's origin point in window space (XY on range [-1,+1]).
    p2      The bar's end point.
    emitDir     Particles will be launched in this direction evenly along the bar.
    minVel      The minimum velocity for particles being emitted.
    maxVel      The maximum emission velocity.
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
ParticleEmitterBar::ParticleEmitterBar(const glm::vec2 &p1, const glm::vec2 &p2, 
    const glm::vec2 &emitDir, float minVel, const float maxVel)
{
    // the start and end points should be translatable
    _start = glm::vec4(p1, 0.0f, 1.0f);
    _end = glm::vec4(p2, 0.0f, 1.0f);
    _minVel = minVel;
    _deltaVelocity = maxVel - minVel;

    // emission direction should not be translatable; like a normal, it should only be rotatable
    _emitDir = glm::vec4(emitDir, 0.0f, 0.0f);

    // the transformed variants begin equal to the original points, then diverge after 
    // SetTransform(...) is called
    _transformedStart = _start;
    _transformedEnd = _end;
    _transformedEmitDir = _emitDir;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the start vertex of the bar.  Used by the compute updater to tell the 
    compute shader where the bar's starting point is.
Parameters: None
Returns:    
    A vec4.
Creator:    John Cox (9-20-2016)
------------------------------------------------------------------------------------------------*/
glm::vec4 ParticleEmitterBar::GetBarStart() const
{
    return _transformedStart;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the start vertex of the bar.  Used by the compute updater.
Parameters: None
Returns:
    A vec4.
Creator:    John Cox (9-20-2016)
------------------------------------------------------------------------------------------------*/
glm::vec4 ParticleEmitterBar::GetBarEnd() const
{
    return _transformedEnd;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emision direction.  Used by the compute updater.
Parameters: None
Returns:
    A vec4.
Creator:    John Cox (9-20-2016)
------------------------------------------------------------------------------------------------*/
glm::vec4 ParticleEmitterBar::GetEmitDir() const
{
    return _transformedEmitDir;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emitter's minimum velocity.  
Parameters: None
Returns:
    A float.
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
float ParticleEmitterBar::GetMinVelocity() const
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
float ParticleEmitterBar::GetDeltaVelocity() const
{
    return _deltaVelocity;
}

/*------------------------------------------------------------------------------------------------
Description:
    Why transform this for every emission of every particle when I can do it once before
    particle updating and be done with it for the rest of the frame?
Parameters:
    emitterTransform    Transform the bar's end points and emit direction with this.
Returns:    None
Exception:  Safe
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
void ParticleEmitterBar::SetTransform(const glm::mat4 &emitterTransform)
{
    _transformedStart = emitterTransform * _start;
    _transformedEnd = emitterTransform * _end;
    _transformedEmitDir = emitterTransform * _emitDir;
}
