#pragma once

#include <string>


/*------------------------------------------------------------------------------------------------
Description:
    Encapsulates quad tree resetting via compute shader:

    For each node:
    - number current particles = 0
    - "is subdivided" = 0
    - child node indices set to -1 (max unsigned int)
    - if the node is not one of the starting nodes, "in use" = 0
Creator:    John Cox (1-16-2017)
------------------------------------------------------------------------------------------------*/
class ComputeQuadTreeGenerateGeometry
{
public:
    ComputeQuadTreeGenerateGeometry(unsigned int maxNodes, unsigned int maxPolygonFaces, const std::string &computeShaderKey);
    ~ComputeQuadTreeGenerateGeometry();

    void GenerateGeometry();
    unsigned int NumActiveFaces() const;

private:
    unsigned int _computeProgramId;
    unsigned int _totalNodes;
    unsigned int _facesInUse;

    unsigned int _atomicCounterBufferId;
    unsigned int _acOffsetPolygonFacesInUse;
    unsigned int _acOffsetPolygonFacesCrudeMutex;

    unsigned int _atomicCounterCopyBufferId;

    int _unifLocMaxNodes;
    int _unifLocMaxPolygonFaces;
};
