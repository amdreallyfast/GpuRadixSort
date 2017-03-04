#pragma once

#include "Include/SSBOs/SsboBase.h"
#include "Include/PolygonFace.h"
#include <vector>

/*------------------------------------------------------------------------------------------------
Description:
    Sets up the Shader Storage Block Object for a 2D polygon.  The polygon will be used in both 
    the compute shader and in the geometry render shader.
Creator:    John Cox, 9-8-2016
------------------------------------------------------------------------------------------------*/
class PolygonSsbo : public SsboBase
{
public:
    PolygonSsbo(const std::vector<PolygonFace> &faceCollection);
    virtual ~PolygonSsbo();

    void ConfigureCompute(unsigned int computeProgramId, const std::string &bufferNameInShader) override;
    void ConfigureRender(unsigned int renderProgramId, unsigned int drawStyle) override;
};

