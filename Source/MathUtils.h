#pragma once

#include <cmath>
#include <numbers>

constexpr const float M_PI = std::numbers::pi_v<float>; 

#define M_TWO_PI 2.0f*std::numbers::pi_v<float>
#define M_HALF_PI 0.5f*std::numbers::pi_v<float>

void euclideanToSpherical(const Vector3 & dir, float& azimuth, float& elevation);
void sphericalToEuclidean(float azimuth, float elevation, Vector3& dir);


void getPlanes(Vector4 planes[6], const Matrix& viewProjection, bool normalize = false);
void getPlanes(const BoundingOrientedBox& obb, Vector4 planes[6], Vector3 absPlanes[6]);
void getPoints(const BoundingOrientedBox& obb, Vector3 points[8]);

DirectX::ContainmentType insidePlanes(const Vector4 planes[6], const Vector3 absPlanes[6], const BoundingBox& box);
DirectX::ContainmentType insidePlanes(const Vector4 planes[6], const Vector3 absPlanes[6], const BoundingOrientedBox& box);
DirectX::ContainmentType insideAABB(const BoundingBox& aabb, const Vector3 points[8]);
