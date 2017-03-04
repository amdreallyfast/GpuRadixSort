#include "Include/Particles/ParticleQuadTree.h"

/*------------------------------------------------------------------------------------------------
Description:
    Sets up the initial tree subdivision with _NUM_ROWS_IN_TREE_INITIAL by 
    _NUM_COLUMNS_IN_TREE_INITIAL nodes.  Boundaries are determined by the particle region center 
    and the particle region radius.  The ParticleUpdater should constrain particles to this 
    region, and the quad tree will subdivide within this region.
Parameters:
    particleRegionCenter    In world space
    particleRegionRadius    In world space
Returns:    None
Exception:  Safe
Creator:    John Cox (12-17-2016)
------------------------------------------------------------------------------------------------*/
ParticleQuadTree::ParticleQuadTree(const glm::vec4 &particleRegionCenter, float particleRegionRadius)
{
    // allocate space for the nodes
    _allQuadTreeNodes.resize(_MAX_NODES);

    _particleRegionCenter = particleRegionCenter;
    _particleRegionRadius = particleRegionRadius;

    // starting at the origin (upper left corner of the 2D particle region)
    // Note: Unlike most rectangles in graphical programming, OpenGL's begin at the lower left.  
    // Beginning at the upper left, as this algorithm does, means that X and Y are not (0,0), 
    // but X and Y are (0, max).
    float xBegin = particleRegionCenter.x - particleRegionRadius;
    float yBegin = particleRegionCenter.y + particleRegionRadius;

    float xIncrementPerNode = 2.0f * particleRegionRadius / _NUM_COLUMNS_IN_TREE_INITIAL;
    float yIncrementPerNode = 2.0f * particleRegionRadius / _NUM_ROWS_IN_TREE_INITIAL;

    float y = yBegin;
    for (int row = 0; row < _NUM_ROWS_IN_TREE_INITIAL; row++)
    {
        float x = xBegin;
        for (int column = 0; column < _NUM_COLUMNS_IN_TREE_INITIAL; column++)
        {
            int nodeIndex = (row * _NUM_COLUMNS_IN_TREE_INITIAL) + column;
            ParticleQuadTreeNode &node = _allQuadTreeNodes[nodeIndex];
            node._inUse = true;

            // set the borders of the node
            node._leftEdge = x;
            node._rightEdge = x + xIncrementPerNode;
            node._topEdge = y;
            node._bottomEdge = y - yIncrementPerNode;

            // assign neighbors
            // Note: Nodes on the edge of the initial tree have three null neighbors.  There may 
            // be other ways to calculate these, but I chose the following because it is spelled 
            // out verbosely.
            // Ex: a top - row node has null top left, top, and top right neighbors.

            // left edge does not have a "left" neighbor
            if (column > 0)
            {
                node._neighborIndexLeft = nodeIndex - 1;
            }

            // left and top edges do not have a "top left" neighbor
            if (row > 0 && column > 0)
            {
                // "top left" neighbor = current node - 1 row - 1 column
                node._neighborIndexTopLeft = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL - 1;
            }

            // top row does not have a "top" neighbor
            if (row > 0)
            {
                node._neighborIndexTop = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL;
            }

            // right and top edges do not have a "top right" neighbor
            if (row > 0 && column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                // "top right" neighbor = current node - 1 row + 1 column
                node._neighborIndexTopRight = nodeIndex - _NUM_COLUMNS_IN_TREE_INITIAL + 1;
            }

            // right edge does not have a "right" neighbor
            if (column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                node._neighborIndexRight = nodeIndex + 1;
            }

            // right and bottom edges do not have a "bottom right" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1) &&
                column < (_NUM_COLUMNS_IN_TREE_INITIAL - 1))
            {
                // "bottom right" neighbor = current node + 1 row + 1 column
                node._neighborIndexBottomRight = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL + 1;
            }

            // bottom edge does not have a "bottom" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1))
            {
                node._neighborIndexBottom = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL;
            }

            // left and bottom edges do not have a "bottom left" neighbor
            if (row < (_NUM_ROWS_IN_TREE_INITIAL - 1) && column > 0)
            {
                // bottom left neighbor = current node + 1 row - 1 column
                node._neighborIndexBottomLeft = nodeIndex + _NUM_COLUMNS_IN_TREE_INITIAL - 1;
            }


            // setup for next node
            x += xIncrementPerNode;
        }

        // end of row, increment to the next one
        y -= yIncrementPerNode;
    }
}
