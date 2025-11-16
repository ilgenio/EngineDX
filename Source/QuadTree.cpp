#include "Globals.h"
#include "QuadTree.h"

#include "DebugDrawPass.h"

#define QUADTREE_HEIGHT 10.0f

static UINT compact1by1(UINT x) {
    x &= 0x55555555;                   // Keep bits in odd positions
    x = (x ^ (x >> 1)) & 0x33333333;
    x = (x ^ (x >> 2)) & 0x0F0F0F0F;
    x = (x ^ (x >> 4)) & 0x00FF00FF;
    x = (x ^ (x >> 8)) & 0x0000FFFF;
    return x;
}

// Decode Morton index into (x, y)
void mortonDecode(UINT morton, UINT& x, UINT& y) {
    x = compact1by1(morton);
    y = compact1by1(morton >> 1);
}

QuadTree::QuadTree()
{

}

QuadTree::~QuadTree()
{

}

bool QuadTree::init(UINT depthLevels, float worldSize)
{
    _ASSERT_EXPR(depthLevels > 0, "Depth levels must be greater than zero.");

    this->depthLevels = depthLevels;
    this->worldHalfSize   = worldSize*0.5f;

    UINT nodeCount = getTreeLength(depthLevels);
    cells.resize(nodeCount);

    UINT levelStartIndex = 0;

    for(UINT depth=0; depth< depthLevels; ++depth)
    {
        // 4^depth
        UINT nodesAtLevel = 1 << (2 * depth); 

        for(UINT i=0; i< nodesAtLevel; ++i)
        {
            UINT nodeIndex = levelStartIndex + i;
            BoundingBox& cell = cells[nodeIndex];

            // Calculate boundaries for the node
            float size = worldSize / float(1 << depth);

            UINT row, col;

            mortonDecode(i, row, col);

            cell.Center  = Vector3(-worldHalfSize + (col + 0.5f) * size, 0.0f, -worldHalfSize + (row + 0.5f) * size);
            cell.Extents = Vector3(size *0.5f, QUADTREE_HEIGHT, size *0.5f);
        }

        levelStartIndex += nodesAtLevel;
        
    }

    return nodeCount > 0;

}

UINT QuadTree::getTreeLength(UINT levels) const
{
    UINT length = 0;
    for (UINT i = 0; i < levels; ++i)
    {
        // 4^depth
        length += (1 << (2 * i));
    }
    return length;
}

UINT QuadTree::computeCellIndex(const BoundingOrientedBox& box) const
{
    UINT levelIndex = 0;
    UINT levelStartIndex = 0; 
    UINT childStartIndex = 0;
    UINT parentIndex = UINT(cells.size());

    Vector3 points[8];
    getPoints(box, points);

    // Root node
    ContainmentType contains = insideAABB(cells[0], points); 

    if (contains == ContainmentType::INTERSECTS)
    {
        return 0;
    }
    else if (contains == ContainmentType::DISJOINT)
    {
        // The box is not fully contained in the root node
        return UINT(cells.size());
    }
    else
    {
        levelIndex++;
        childStartIndex = 1;
        levelStartIndex = 1;
        parentIndex = 0;
    }

    while(levelIndex < depthLevels)
    {
        UINT i = 0;
        for (; i < 4; ++i)
        {
            UINT nodeIndex = childStartIndex + i;

            ContainmentType contains = insideAABB(cells[nodeIndex], points);

            if(contains == ContainmentType::CONTAINS) 
            {
                levelIndex++;

                if(levelIndex+1 < depthLevels)
                {
                    // Go deeper
                    UINT nodesAtLevel = 1 << (2 * (levelIndex-1));

                    parentIndex      = nodeIndex;
                    childStartIndex  = levelStartIndex + nodesAtLevel + (nodeIndex - levelStartIndex) * 4;
                    levelStartIndex += nodesAtLevel;
                    break;
                }
                else
                {
                    // Ended in a leaf node
                    return nodeIndex;
                }
            }
            else if (contains == ContainmentType::INTERSECTS)
            {
                // Return always the deepest fully containing the box
                return parentIndex;
            }
        }

        if(i == 4)
        {
            _ASSERT_EXPR(levelIndex == 0, L"Only the root node can not contain or intersect the box.");

            // No child contains the box
            break;
        }
    }

    return UINT(cells.size());
}

void QuadTree::frustumCulling(const Vector4 frustumPlanes[6], std::vector<ContainmentType>& containment) const
{
    containment.resize(cells.size(), ContainmentType::DISJOINT);

    Vector3 absFrustumPlanes[6];
    for(int i=0; i<6; ++i)
    {
        // Precompute absolute plane normals for faster box-frustum tests
        absFrustumPlanes[i] = Vector3(std::abs(frustumPlanes[i].x), std::abs(frustumPlanes[i].y), std::abs(frustumPlanes[i].z));
    }

    UINT levelIndex = 0;
    UINT levelStartIndex = 0;
    UINT parentLevelStartIndex = 0;

    while(levelIndex < depthLevels)
    {
        UINT nodesAtLevel = 1 << (2 * levelIndex);

        for (UINT i = 0; i < nodesAtLevel; ++i)
        {
            UINT nodeIndex = levelStartIndex + i;
            const BoundingBox& cell = cells[nodeIndex];

            if(levelIndex == 0)
            {
                // Root node
                containment[nodeIndex] = insidePlanes(frustumPlanes, absFrustumPlanes, cell);
            }
            else
            {
                // Child nodes
                UINT parentIndex = parentLevelStartIndex + (i >> 2);
                ContainmentType parentContainment = containment[parentIndex];

                if (parentContainment == ContainmentType::CONTAINS) // CONTAINS
                {
                    containment[nodeIndex] = ContainmentType::CONTAINS;
                }
                else if(parentContainment == ContainmentType::INTERSECTS) // INTERSECTS
                {
                    containment[nodeIndex] = insidePlanes(frustumPlanes, absFrustumPlanes, cell);
                }
            }
        }

        parentLevelStartIndex = levelStartIndex;
        levelStartIndex += nodesAtLevel;
        ++levelIndex;
    }
}

void QuadTree::debugDraw(const std::vector<ContainmentType> &containment) const
{
    UINT levelIndex = 0;
    UINT levelStartIndex = 0;
    UINT nodesAtLevel = 1 << (2 * levelIndex);

    while(levelIndex+1 < depthLevels)
    {
        levelStartIndex += nodesAtLevel;
        ++levelIndex;
        nodesAtLevel = 1 << (2 * levelIndex);
    }

    // Draw only the last level

    for (UINT i = 0; i < nodesAtLevel; ++i)
    {
        const BoundingBox& cell = cells[levelStartIndex + i];
        switch (containment[levelStartIndex + i])
        {
        case ContainmentType::CONTAINS:
            dd::box(ddConvert(cell.Center), dd::colors::Green, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        case ContainmentType::INTERSECTS:
            dd::box(ddConvert(cell.Center), dd::colors::Yellow, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        case ContainmentType::DISJOINT:
            dd::box(ddConvert(cell.Center), dd::colors::Red, cell.Extents.x * 2.0f, cell.Extents.y * 2.0f, cell.Extents.z * 2.0f, 0, false);
            break;
        }
    }
}