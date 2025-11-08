#pragma once

#include <vector>

class QuadTree
{
    UINT depthLevels = 0;
    float worldHalfSize = 0.0f;

    std::vector<BoundingBox> cells;

public:

    QuadTree();
    ~QuadTree();

    bool init(UINT depthLevels, float worldSize);

    UINT computeCellIndex(const BoundingOrientedBox& box) const;
    void frustumCulling(const Vector4 planes[6], std::vector<ContainmentType>& containment) const;
    void debugDraw(const std::vector<ContainmentType>& containment) const;

    UINT getCellCount() const { return static_cast<UINT>(cells.size()); }

private:

    UINT getTreeLength(UINT levels) const;

};