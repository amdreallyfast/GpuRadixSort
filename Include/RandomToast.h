#pragma once

#include "ThirdParty/glm/vec3.hpp"

/*------------------------------------------------------------------------------------------------
Description:
    Both the "min max velocity" and "particle emiter bar" objects use randomness, so rather than
    have them both attempting to seed random, I decided to put the randomness handling off on its
    own, and while I was at it, I decided to put in some randomness functions that have become 
    handy while working out the particle emission and min-max velocity problems.

    This could have a class, but it would have had to have been a singleton or static or had a
    global, and the whole point of that would have been to make sure that randomness was seeded.
    Alternately, I could just make them functions and use a "fast random" algorithm that I found 
    online that doesn't require initialization.  I'll do that.

    And why the "Toast" in the file name?  Because this is a randomness handler and I thought of 
    toast.  2 + 2 = toast, obviously.
Creator:    John Cox (6-25-2016)
------------------------------------------------------------------------------------------------*/

float RandomOnRange0to1();
unsigned long Random();
long RandomPosAndNeg();
glm::vec3 RandomColor();
