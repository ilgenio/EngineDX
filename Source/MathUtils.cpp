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
void getFrustumPlanes(Vector4 planes[6], const Matrix& viewProjection, bool normalize /*= false*/)
{
    Matrix vp = viewProjection.Transpose();

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
    planes[3].y = vp._34 + vp._32;
    planes[3].z = vp._43 + vp._23;
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

DirectX::ContainmentType insideFrustum(const Vector4 planes[6], const BoundingBox& box)
{
    Vector4 center(box.Center.x, box.Center.y, box.Center.z, -1.0f);
    const Vector3& extents = *reinterpret_cast<const Vector3*>(&box.Extents);

    bool allInside = true;

    for (int i = 0; i < 6; ++i)
    {
        float dist = fabsf(center.Dot(planes[i]));
        float radius = extents.Dot(Vector3(fabsf(planes[i].x), fabsf(planes[i].y), fabsf(planes[i].z)));

         if(dist > radius) return DirectX::ContainmentType::DISJOINT;

        allInside &= dist < -radius;
    }

     return allInside ? DirectX::ContainmentType::CONTAINS : DirectX::ContainmentType::INTERSECTS;
}

bool insideFrustum(const Vector4 planes[6], const BoundingOrientedBox& box)
{
    int out;
    for (int i = 0; i < 6; ++i)
    {
        out = 0;
        for (int k = 0; k < 8; ++k)
            //out += planes[i].IsOnPositiveSide(points[k]);

            if (out == 8)
                return false;
    }

    return true;
}


