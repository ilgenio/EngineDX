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