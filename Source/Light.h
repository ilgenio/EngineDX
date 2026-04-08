#pragma once

#include <variant>

// Directional light (e.g., sunlight).
// Casts parallel rays in a single direction, simulating infinitely distant light sources.
struct Directional
{
    Vector3 Ld;       // Light direction vector (normalized)
    Color Lc;         // Light color (RGB) + intensity (alpha)

    Directional() : Ld(Vector3::UnitX), Lc(Vector3::One) {}
    Directional(const Vector3& direction, const Color& color) : Ld(direction), Lc(color) {}
};

// Point light (omnidirectional).
// Emits light equally in all directions from a single point in space.
// Light attenuates based on distance using the squared radius for falloff.
struct Point
{
    Vector3 Lp;         // Light position in world space
    float   sqRadius;   // Squared radius for attenuation falloff
    Color   Lc;         // Light color (RGB) + intensity (alpha )

    Point() : Lp(Vector3::Zero), sqRadius(1.0f), Lc(Vector3::One) {}
    Point(const Vector3& position, float radius, const Color& color) : Lp(position), sqRadius(radius* radius), Lc(color) {}
};

// Spot light (conical).
// Emits light in a cone shape from a position in a specific direction.
// Has inner and outer cone angles for smooth falloff at the edges.
struct Spot
{
    Vector3 Ld;         // Light direction vector (normalized)
    float  sqRadius;    // Squared radius for distance attenuation
    Vector3 Lp;         // Light position in world space
    float  inner;       // Inner cone angle (cosine) - full intensity
    Color   Lc;         // Light color (RGB) + intensity (alpha)
    float  outer;       // Outer cone angle (cosine) - falloff boundary

    Spot() : Ld(Vector3::UnitX), sqRadius(1.0f), Lp(Vector3::Zero), inner(0.5f), Lc(Vector3::One), outer(1.0f) {}
    Spot(const Vector3& direction, float radius, const Vector3& position, float innerAngle, float outerAngle, const Color& color)
        : Ld(direction), sqRadius(radius* radius), Lp(position), inner(innerAngle), Lc(color), outer(outerAngle) {
    }
};

// Enum identifying the type of light stored in the union
enum ELightType
{
    LIGHT_DIRECTIONAL = 0,    // Directional light (parallel rays)
    LIGHT_POINT,              // Point light (omnidirectional)
    LIGHT_SPOT                // Spot light (conical)
};

class Scene;

// Main light structure supporting multiple light types (directional, point, spot).
// Uses a union to store different light type data, with an enum to identify the active type.
class Light
{

    union LightData
    {
        Directional* directional;
        Point* point;
        Spot* spot;
    } data;

    ELightType type;
    Scene* scene = nullptr;

public:

    ~Light();

    ELightType          getType() const { return type; }

    const Directional&  getDirectional() const { _ASSERTE(getType() == LIGHT_DIRECTIONAL); return *data.directional; }
    const Point&        getPoint() const { _ASSERTE(getType() == LIGHT_POINT); return *data.point; }
    const Spot&         getSpot() const { _ASSERTE(getType() == LIGHT_SPOT); return *data.spot; }

    void setDirectional(const Directional& directional) {  type = LIGHT_DIRECTIONAL; *data.directional = directional; }
    void setPoint(const Point& point) { type = LIGHT_POINT; *data.point = point; }
    void setSpot(const Spot& spot) { type = LIGHT_SPOT; *data.spot = spot; } 

private:
    friend class Scene;

    Light(Directional* directional, Scene* scene) : scene(scene) { type = LIGHT_DIRECTIONAL; data.directional = directional; }
    Light(Point* point, Scene* scene) : scene(scene) { type = LIGHT_POINT; data.point = point; }
    Light(Spot* spot, Scene* scene) : scene(scene) { type = LIGHT_SPOT; data.spot = spot; }

    Light() : scene(nullptr) { data.directional = nullptr; }
};