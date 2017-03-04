#include "Include/MinMaxVelocity.h"

#include "ThirdParty/glm/detail/func_geometric.hpp" // for normalizing glm vectors
#include "Include/RandomToast.h"


/*------------------------------------------------------------------------------------------------
Description:
    Ensures that the object starts object with initialized values.
Parameters: None
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
MinMaxVelocity::MinMaxVelocity() :
    _min(0.0f),
    _velocityDelta(0.0f),
    _useRandomDir(true) // until a direction is set, assume random
{
}

/*------------------------------------------------------------------------------------------------
Description:
    Initializes the magnitudes of the minimum veocity and the velocity delta.
Parameters: 
    min     The minimum velocity in window space (XY on range [-1,+1].
    max     The maximum velocity in window space.
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
void MinMaxVelocity::SetMinMaxVelocity(const float min, const float max)
{
    _min = min;
    _velocityDelta = max - min;
}

/*------------------------------------------------------------------------------------------------
Description:
    Initializes the minimum and delta velocity directions.  Normalizes the provided value, so the
    user does not need to normalize the direction themselves.
Parameters:
    dir     Self-explanatory.
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
void MinMaxVelocity::SetDir(const glm::vec2 &dir)
{
    _dir = glm::normalize(glm::vec4(dir, 0.0f, 1.0f));
    _useRandomDir = false;
}

/*------------------------------------------------------------------------------------------------
Description:
    Tells the object to use a random velocity direction every time "get new" is called instead of
    the provided direction.
Parameters: None
Returns:    None
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
void MinMaxVelocity::UseRandomDir()
{
    _dir = glm::vec4();
    _useRandomDir = true;
}

/*------------------------------------------------------------------------------------------------
Description:
    Generates a new velocity vector between the previously provided minimum and maximum values
    (or 0 if nothing was set after this object was instatiated) and in the provided direction 
    (or a random direction if no direction was set).
Parameters: None
Returns:    
    A 2D vector whose magnitude is between the initialized "min" and "max" values and whose 
    direction is random.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
glm::vec4 MinMaxVelocity::GetNew() const
{
    //float velocityVariation = ((float)rand() * INVERSE_RAND_MAX) * _velocityDelta;
    float velocityVariation = RandomOnRange0to1() * _velocityDelta;
    float velocityMagnitude = _min + velocityVariation;
    
    if (_useRandomDir)
    {
        // create a normalized random vector, then multiple all items by the magnitude
        // Note: The hard-coded mod100 is just to prevent the random axis magnitudes from 
        // getting too crazy different from each other.
        float newX = (float)(RandomPosAndNeg() % 100);
        float newY = (float)(RandomPosAndNeg() % 100);
        glm::vec4 randomVelocityVector = glm::normalize(glm::vec4(newX, newY, 0.0f, 0.0f));
        return (randomVelocityVector * velocityMagnitude);
    }
    else  // read, "don't use random direction"
    {
        return (_dir * velocityMagnitude);
    }
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emitter's minimum velocity.  It is used by
    ParticlePolygonComputeUpdater.cpp to tell the compute shader how fast particles should be
    spit out of this emitter.
Parameters: None
Returns:
    A float.
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
float MinMaxVelocity::GetMinVelocity() const
{
    return _min;
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple getter for the emitter's delta velocity.  It is used by
    ParticlePolygonComputeUpdater.cpp to tell the compute shader the velocity range that reset
    particles can be varied over for this emitter.
Parameters: None
Returns:
A float.
Creator:    John Cox (10-10-2016)
------------------------------------------------------------------------------------------------*/
float MinMaxVelocity::GetDeltaVelocity() const
{
    return _velocityDelta;
}
