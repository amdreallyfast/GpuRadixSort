#pragma once

#include "Include/SSBOs/SsboBase.h"


/*------------------------------------------------------------------------------------------------
Description:
    A slightly-more-than-base-case convenience.

    In another demo, this would be ParticleSsbo and would also define ConfigureRender.
Creator:    John Cox, 3/2017
------------------------------------------------------------------------------------------------*/
class OriginalDataSsbo : public SsboBase
{
public:
    OriginalDataSsbo(unsigned int numItems);

    unsigned int NumItems() const;

private:
    unsigned int _numItems;
};