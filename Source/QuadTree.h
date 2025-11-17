#pragma once

#include <vector>

class QuadTree
{
    UINT depthLevels = 0;
    float worldHalfSize = 0.0f;

    struct Cell
    {
        BoundingBox bbox;
        UINT numObjects = 0;
        UINT depthLevel = 0;
    };

    std::vector<Cell> cells;
    std::vector<UINT> treeLengths;

public:

    QuadTree();
    ~QuadTree();

    bool init(UINT depthLevels, float worldSize);

    UINT computeCellIndex(const BoundingOrientedBox& box) const;
    void frustumCulling(const Vector4 planes[6], const Vector3 absPlanes[6], std::vector<IntersectionType>& containment) const;
    void debugDraw(const std::vector<IntersectionType>& containment, UINT level) const;

    UINT getCellCount() const { return static_cast<UINT>(cells.size()); }
    UINT getDepthLevels() const { return depthLevels; }

    void addObject(UINT index);
    void removeObject(UINT index);

private:

    UINT getLevelStartIndex(UINT level) const
    {
        _ASSERTE(level < depthLevels);
        return level == 0 ? 0 : treeLengths[level - 1];
    }

    UINT getNodesAtLevel(UINT level) const
    {
        _ASSERTE(level < depthLevels);
        // 4^level
        return 1 << (2 * level);
    }

};