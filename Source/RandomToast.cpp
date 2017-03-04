#include "Include/RandomToast.h"
#include <climits>

// initial values for xorshf96()
static unsigned long x = 123456789, y = 362436069, z = 521288629;

/*------------------------------------------------------------------------------------------------
Description:
    Marsaglia's xorshf generator.  According to 
    http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c, unless I'm 
    doing cryptography, this generator rocks.  It is apparently very good for doing loads of 
    randomness at runtime, and I've read that it is notably faster than rand().
    
    Also, rand() is getting discouraged in C++14, likely because it provides 16 bits of 
    randomness, which is more than good enough for many applications, but since it is getting 
    old, I might as well get a newer and faster generator.
    http://stackoverflow.com/questions/22600100/why-are-stdshuffle-methods-being-deprecated-in-c14

    The algorithm operates on all of the 64bits in the long integer.  It is positive if stuck 
    into an unsigned long but can be negative if stuck into a signed long because the most 
    significant bit (the signed bit) might be 1.
Parameters: None
Returns:    
    A decently chaotic unsigned long (64bit) number.
Creator:    Some online dude named Marsaglia (unknown date).
------------------------------------------------------------------------------------------------*/
static unsigned long xorshf96(void) {          //period 2^96-1
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
}

// used for fast (faster than dividing, at least) reduction to the range [0,+1]
static const float INVERSE_UNSIGNED_LONG = 1.0f / ULONG_MAX;


/*------------------------------------------------------------------------------------------------
Description:
    Generates a random positve float on the range [0,+1].
Parameters: None
Returns:    
    See description.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
float RandomOnRange0to1()
{
    return ((float)xorshf96() * INVERSE_UNSIGNED_LONG);
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple encapsulation for Marsaglia xorshf96() that generates a positive random long integer
    without exposing how.
Parameters: None
Returns:
    See description.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
unsigned long Random()
{
    return xorshf96();
}

/*------------------------------------------------------------------------------------------------
Description:
    A simple encapsulation for Marsaglia xorshf96() that generates a random long integer without
    exposing how and may be positive or negative.
Parameters: None
Returns:
    See description.
Creator:    John Cox (7-2-2016)
------------------------------------------------------------------------------------------------*/
long RandomPosAndNeg()
{
    return (long)(xorshf96());
}

/*------------------------------------------------------------------------------------------------
Description:
    Generates three random floats on the range 0.0f - 1.0f and stores them in a glm::vec3 for
    use in RGB or other red-green-blue color combinations (texture format and internal format can
    be RGB, GBR, GRB, etc.).
Parameters: None
Returns:
    A glm::vec3 with three random floats stuffed inside on the range 0.0f - 1.0f;
Creator:    John Cox (6-12-2016)
------------------------------------------------------------------------------------------------*/
glm::vec3 RandomColor()
{
    glm::vec3 ret;
    ret.x = RandomOnRange0to1();
    ret.y = RandomOnRange0to1();
    ret.z = RandomOnRange0to1();
    return ret;
}
