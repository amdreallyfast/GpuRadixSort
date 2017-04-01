#pragma once

#include "Include/SSBOs/SsboBase.h"


/*------------------------------------------------------------------------------------------------
Description:
    Same as the OriginalDataSsbo, but called by a different name because it is a different 
    buffer that is meant exclusively for use during sorting (sorting happens in parallel, so 
    need to copy the original data and then have each thread copy the copied data back to the 
    original place).

    Note: If it is possible to only operate on the sorted data (ex: sorting particles by morton 
    codes so that a bounding volume hierarchy can be constructed only requires the sorted values 
    and how to find the original particles, not sorting the particles themselves), then this 
    structure will not be necessary.

    In another demo, this would be ParticleSsbo and would also define ConfigureRender.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class OriginalDataCopySsbo : public SsboBase
{
public:
    OriginalDataCopySsbo(unsigned int numItems);
    typedef std::shared_ptr<OriginalDataCopySsbo> SHARED_PTR;

private:
};