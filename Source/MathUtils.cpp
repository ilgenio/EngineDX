#include "Globals.h"
#include "Math.h"

void euclideanToSpherical(const Vector3 & dir, float& azimuth, float& elevation)
{
    Vector3 norm = dir;
    norm.Normalize();
    azimuth = atan2f(norm.z, norm.x);
    elevation = acosf(norm.y);
};

void sphericalToEuclidean(float azimuth, float elevation, Vector3& dir)
{
    float cos_azimuth = cosf(azimuth);
    float sin_azimuth = sinf(azimuth);
    float cos_elevation = cosf(elevation);
    float sin_elevation = sinf(elevation);

    dir.x = sin_elevation * cos_azimuth;
    dir.z = sin_elevation * sin_azimuth;
    dir.y = cos_elevation;
};

// From https://www8.cs.umu.se/kurser/5DV051/HT12/lab/plane_extraction.pdf
void getPlanes(Vector4 planes[6], const Matrix& viewProjection, bool normalize /*= false*/)
{
    Matrix vp = viewProjection;
    vp.Transpose();

    // Left clipping plane
    planes[0].x = vp._14 + vp._11;
    planes[0].y = vp._24 + vp._21;
    planes[0].z = vp._34 + vp._31;
    planes[0].w = vp._44 + vp._41;

    // Right clipping plane
    planes[1].x = vp._14 - vp._11;
    planes[1].y = vp._24 - vp._21;
    planes[1].z = vp._34 - vp._31;
    planes[1].w = vp._44 - vp._41;

    // Top clipping plane
    planes[2].x = vp._14 - vp._12;
    planes[2].y = vp._24 - vp._22;
    planes[2].z = vp._34 - vp._32;
    planes[2].w = vp._44 - vp._42;

    // Bottom clipping plane
    planes[3].x = vp._14 + vp._12;
    planes[3].y = vp._24 + vp._22;
    planes[3].z = vp._34 + vp._32;
    planes[3].w = vp._44 + vp._42;

    // Near clipping plane
    planes[4].x = vp._13;
    planes[4].y = vp._23;
    planes[4].z = vp._33;
    planes[4].w = vp._43;

    // Far clipping plane
    planes[5].x = vp._14 - vp._13;
    planes[5].y = vp._24 - vp._23;
    planes[5].z = vp._34 - vp._33;
    planes[5].w = vp._44 - vp._43;

    if (normalize) // for frustum culling is not needed
    {
        for (int i = 0; i < 6; ++i)
        {            
            float len = Vector3(&planes[i].x).Length();
            _ASSERTE(len > 0.0f);
            planes[i] /= len;
        }
    }

}

void getPlanes(const BoundingOrientedBox& obb, Vector4 planes[6], Vector3 absPlanes[6])
{
    Vector3::Transform(Vector3::UnitX, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[0]);
    Vector3::Transform(-Vector3::UnitX, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[1]);

    Vector3::Transform(Vector3::UnitY, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[2]);
    Vector3::Transform(-Vector3::UnitY, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[3]);

    Vector3::Transform(Vector3::UnitZ, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[4]);
    Vector3::Transform(-Vector3::UnitZ, *reinterpret_cast<const Quaternion*>(&obb.Orientation), absPlanes[5]);


    for (int i = 0; i < 6; ++i)
    {
        Vector3 P = obb.Center + absPlanes[0] * obb.Extents.x;
        planes[i] = Vector4(absPlanes[i].x, absPlanes[i].y, absPlanes[i].z, -absPlanes[0].Dot(P));

        absPlanes[i].x = fabsf(absPlanes[i].x);
        absPlanes[i].y = fabsf(absPlanes[i].y);
        absPlanes[i].z = fabsf(absPlanes[i].z);
    }
}

void getPoints(const BoundingOrientedBox& obb, Vector3 points[8])
{
    Vector3 axis[3];
    Vector3::Transform(Vector3::UnitX, *reinterpret_cast<const Quaternion*>(&obb.Orientation), axis[0]);
    Vector3::Transform(Vector3::UnitY, *reinterpret_cast<const Quaternion*>(&obb.Orientation), axis[1]);
    Vector3::Transform(Vector3::UnitZ, *reinterpret_cast<const Quaternion*>(&obb.Orientation), axis[2]);

    
    points[0] = obb.Center + axis[0] * obb.Extents.x + axis[1] * obb.Extents.y + axis[2] * obb.Extents.z;
    points[1] = obb.Center - axis[0] * obb.Extents.x + axis[1] * obb.Extents.y + axis[2] * obb.Extents.z;
    points[2] = obb.Center + axis[0] * obb.Extents.x - axis[1] * obb.Extents.y + axis[2] * obb.Extents.z;
    points[3] = obb.Center + axis[0] * obb.Extents.x + axis[1] * obb.Extents.y - axis[2] * obb.Extents.z;
    points[4] = obb.Center - axis[0] * obb.Extents.x - axis[1] * obb.Extents.y + axis[2] * obb.Extents.z;
    points[5] = obb.Center + axis[0] * obb.Extents.x - axis[1] * obb.Extents.y - axis[2] * obb.Extents.z;
    points[6] = obb.Center - axis[0] * obb.Extents.x + axis[1] * obb.Extents.y - axis[2] * obb.Extents.z;
    points[7] = obb.Center - axis[0] * obb.Extents.x - axis[1] * obb.Extents.y - axis[2] * obb.Extents.z;
}

IntersectionType insidePlanes(const Vector4 planes[6], const Vector3 absPlanes[6], const BoundingBox& box)
{
    Vector4 center(box.Center.x, box.Center.y, box.Center.z, 1.0f);
    const Vector3& extents = *reinterpret_cast<const Vector3*>(&box.Extents);

    bool allInside = true;

    for (int i = 0; i < 6; ++i)
    {
        float dist = center.Dot(planes[i]);
        float radius = extents.Dot(absPlanes[i]);

        if (dist + radius < 0.0f)  return OUTSIDE;

        allInside &= dist - radius >= 0.0f;
    }

    return allInside ? INSIDE : INTERSECTION;

}

IntersectionType insidePlanes(const Vector4 planes[6], const Vector3 absPlanes[6], const BoundingOrientedBox& box)
{
    Vector4 center(box.Center.x, box.Center.y, box.Center.z, 1.0f);
    const Vector3& extents = *reinterpret_cast<const Vector3*>(&box.Extents);
    
    Vector3 axis[3];
    Vector3::Transform(Vector3::UnitX, *reinterpret_cast<const Quaternion*>(&box.Orientation), axis[0]);
    Vector3::Transform(Vector3::UnitY, *reinterpret_cast<const Quaternion*>(&box.Orientation), axis[1]);
    Vector3::Transform(Vector3::UnitZ, *reinterpret_cast<const Quaternion*>(&box.Orientation), axis[2]);

    const Vector3 orientedExtents(
        fabsf(axis[0].x) * extents.x + fabsf(axis[1].x) * extents.y + fabsf(axis[2].x) * extents.z,
        fabsf(axis[0].y) * extents.x + fabsf(axis[1].y) * extents.y + fabsf(axis[2].y) * extents.z,
        fabsf(axis[0].z) * extents.x + fabsf(axis[1].z) * extents.y + fabsf(axis[2].z) * extents.z
    );

    bool allInside = true;

    for (int i = 0; i < 6; ++i)
    {
        float dist = center.Dot(planes[i]);
        float radius = orientedExtents.Dot(absPlanes[i]);

        if (dist + radius < 0.0f)  return OUTSIDE;

        allInside &= dist-radius >= 0.0f;
    }

    return allInside ? INSIDE : INTERSECTION;
}

IntersectionType insideAABB(const BoundingBox& aabb, const Vector3 points[8])
{
    Vector3 min = aabb.Center - aabb.Extents;
    Vector3 max = aabb.Center + aabb.Extents;

    bool allInside = true;
    bool anyInside = false;

    for (int i = 0; (allInside || !anyInside) && i < 8; ++i)
    {
        bool inside = points[i].x >= min.x && points[i].x <= max.x &&
            points[i].y >= min.y && points[i].y <= max.y &&
            points[i].z >= min.z && points[i].z <= max.z;

        allInside &= inside;
        anyInside |= inside;
    }

    return allInside ? INSIDE : (anyInside ? INTERSECTION : OUTSIDE);
}

