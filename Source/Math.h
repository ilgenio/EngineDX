#pragma once

#include <cmath>
#include <numbers>

constexpr const float M_PI = std::numbers::pi_v<float>; 

#define M_TWO_PI 2.0f*std::numbers::pi_v<float>
#define M_HALF_PI 0.5f*std::numbers::pi_v<float>

void euclideanToSpherical(const Vector3 & dir, float& azimuth, float& elevation);
void sphericalToEuclidean(float azimuth, float elevation, Vector3& dir);