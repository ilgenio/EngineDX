#include "Globals.h"
#include "QuadTree.h"

#include "DebugDrawPass.h"

static UINT compact1by1(UINT x) 
{
    x &= 0x55555555;                   // Keep bits in odd positions
    x = (x ^ (x >> 1)) & 0x33333333;
    x = (x ^ (x >> 2)) & 0x0F0F0F0F;
    x = (x ^ (x >> 4)) & 0x00FF00FF;
    x = (x ^ (x >> 8)) & 0x0000FFFF;
    return x;
}

// Decode Morton index into (x, y)
static void mortonDecode(UINT morton, UINT& x, UINT& y) 
{
    x = compact1by1(morton);
    y = compact1by1(morton >> 1);
}

QuadTree::QuadTree()
{

}

QuadTree::~QuadTree()
{

}

bool QuadTree::init(UINT depthLevels, float worldSize, float height)
{
    _ASSERT_EXPR(depthLevels > 0, "Depth levels must be greater than zero.");

    this->depthLevels       = depthLevels;
    this->worldHalfSize     = worldSize*0.5f;
    this->treeLengths.resize(depthLevels);

    UINT nodeCount = 0;
    
    for(UINT depth=0; depth< depthLevels; ++depth)
    {
        nodeCount += getNodesAtLevel(depth);
        treeLengths[depth] = nodeCount;
    }

    cells.resize(nodeCount);

    for(UINT depth=0; depth< depthLevels; ++depth)
    {
        // 4^depth
        UINT nodesAtLevel = 1 << (2 * depth); 
        UINT levelStartIndex = getLevelStartIndex(depth);

        for(UINT i=0; i< nodesAtLevel; ++i)
        {
            UINT nodeIndex = levelStartIndex + i;
            Cell& cell = cells[nodeIndex];
            
            // Calculate boundaries for the node
            float size = worldSize / float(1 << depth);

            UINT row, col;

            mortonDecode(i, row, col);

            cell.numObjects   = 0;
            cell.depthLevel   = depth;
            cell.bbox.Center  = Vector3(-worldHalfSize + (col + 0.5f) * size, 0.0f, -worldHalfSize + (row + 0.5f) * size);
            cell.bbox.Extents = Vector3(size *0.5f, height * 0.5f, size *0.5f);
        }
    }

    return nodeCount > 0;

}

UINT QuadTree::computeCellIndex(const BoundingOrientedBox& box) const
{
    Vector3 points[8];
    getPoints(box, points);

    // Root node
    IntersectionType contains = insideAABB(cells[0].bbox, points); 
    if (contains == INTERSECTION) return 0;
    else if (contains == OUTSIDE) return UINT(cells.size());

    UINT childStartIndex = 1;

    for (UINT levelIndex = 1; levelIndex < depthLevels; ++levelIndex)
    {
        UINT i = 0;
        for (; i < 4; ++i)
        {
            UINT nodeIndex = childStartIndex + i;

            IntersectionType contains = insideAABB(cells[nodeIndex].bbox, points);

            if(contains == INSIDE) 
            {
                if(levelIndex+1 < depthLevels)
                {
                    // Go deeper
                    UINT levelStart     = getLevelStartIndex(levelIndex);
                    UINT nextLevelStart = getLevelStartIndex(levelIndex+1);
                    childStartIndex     = nextLevelStart + (nodeIndex - levelStart) * 4;

                    break;
                }
                else
                {
                    // Ended in a leaf node
                    return nodeIndex;
                }
            }
            else if (contains == INTERSECTION)
            {
                UINT levelStart     = getLevelStartIndex(levelIndex);
                UINT prevLevelStart = getLevelStartIndex(levelIndex-1);
                UINT parentIndex    = prevLevelStart + (nodeIndex - levelStart) / 4;

                return parentIndex;
            }
        }

        _ASSERTE(i < 4); // Should have found a child that contains the box
    }

    return UINT(cells.size());
}

void QuadTree::frustumCulling(const Vector4 frustumPlanes[6], const Vector3 absFrustumPlanes[6], std::vector<IntersectionType>& containment) const
{
    containment.resize(cells.size(), OUTSIDE);

    UINT levelIndex = 0;

    for(UINT levelIndex = 0; levelIndex < depthLevels; ++levelIndex)
    {
        UINT nodesAtLevel = getNodesAtLevel(levelIndex);
        UINT levelStartIndex = getLevelStartIndex(levelIndex);

        for (UINT i = 0; i < nodesAtLevel; ++i)
        {
            UINT nodeIndex = levelStartIndex + i;
            const Cell& cell = cells[nodeIndex];

            // empty cell then OUTSIDE
            if (cell.numObjects > 0)
            {
                // Root node
                if (levelIndex == 0)
                {
                    containment[nodeIndex] = insidePlanes(frustumPlanes, absFrustumPlanes, cell.bbox);
                }
                else
                {
                    UINT parentLevelStartIndex = getLevelStartIndex(levelIndex-1);
                    UINT parentIndex = parentLevelStartIndex + (i >> 2);

                    IntersectionType parentContainment = containment[parentIndex];

                    if (parentContainment == INSIDE) 
                    {
                        containment[nodeIndex] = INSIDE;
                    }
                    else if (parentContainment == INTERSECTION)
                    {
                        containment[nodeIndex] = insidePlanes(frustumPlanes, absFrustumPlanes, cell.bbox);
                    }
                }
            }
        }
    }
}

void QuadTree::debugDraw(const std::vector<IntersectionType> &containment, UINT level) const
{
    level = std::min(level, depthLevels - 1);
    UINT levelStartIndex = getLevelStartIndex(level);
    UINT nodesAtLevel    = getNodesAtLevel(level);

    for (UINT i = 0; i < nodesAtLevel; ++i)
    {
        const BoundingBox& cell = cells[levelStartIndex + i].bbox;
        switch (containment[levelStartIndex + i])
        {
        case INSIDE:
            dd::box(ddConvert(cell.Center), dd::colors::Green, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        case INTERSECTION:
            dd::box(ddConvert(cell.Center), dd::colors::Yellow, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        case OUTSIDE:
            dd::box(ddConvert(cell.Center), dd::colors::Red, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        }
    }
}

void QuadTree::addObject(UINT index) 
{ 
    UINT level = cells[index].depthLevel;
    while (level >= 0)
    {
        ++cells[index].numObjects;

        if (level > 0)
        {
            UINT startIndex = getLevelStartIndex(level);
            UINT prevLevelStartIndex = getLevelStartIndex(level - 1);

            // Move to parent
            index = prevLevelStartIndex + (index - startIndex) / 4;
            --level;
        }
        else
        {
            break;
        }

    }
}

void QuadTree::removeObject(UINT index) 
{
    UINT level = cells[index].depthLevel;

    while (level >= 0)
    {
        _ASSERTE(cells[index].numObjects > 0);
        --cells[index].numObjects;

        if (level > 0)
        {
            UINT startIndex = getLevelStartIndex(level);
            UINT prevLevelStartIndex = getLevelStartIndex(level - 1);

            // Move to parent
            index = prevLevelStartIndex + (index - startIndex) / 4;
            --level;
        }
        else
        {
            break;
        }
    }
}

