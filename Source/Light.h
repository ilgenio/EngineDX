#pragma once

#include <variant>

// Directional light (e.g., sunlight).
// Casts parallel rays in a single direction, simulating infinitely distant light sources.
struct Directional
{
    Vector3 Ld;         // Light direction vector (normalized)
    float   intensity;  // Light intensity multiplier
    Vector3 Lc;         // Light color (RGB)

    Directional() : Ld(Vector3::UnitX), intensity(1.0f), Lc(Vector3::One) {}
};

// Point light (omnidirectional).
// Emits light equally in all directions from a single point in space.
// Light attenuates based on distance using the squared radius for falloff.
struct Point
{
    Vector3 Lp;         // Light position in world space
    float   sqRadius;   // Squared radius for attenuation falloff
    Vector3 Lc;         // Light color (RGB)
    float   intensity;  // Light intensity multiplier

    Point() : Lp(Vector3::Zero), sqRadius(1.0f), Lc(Vector3::One), intensity(1.0f) {}
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
    Vector3 Lc;         // Light color (RGB)
    float  outer;       // Outer cone angle (cosine) - falloff boundary
    float  intensity;   // Light intensity multiplier

    Spot() : Ld(Vector3::UnitX), sqRadius(1.0f), Lp(Vector3::Zero), inner(0.5f), Lc(Vector3::One), outer(1.0f), intensity(1.0f) {}
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

    typedef std::variant<Directional, Point, Spot> LightData; // Alternative using std::variant for type safety

    LightData data;
    Scene* scene = nullptr; 
public:

    Light(const Directional& directional, Scene* scene) : scene(scene) { data = directional; }
    Light(const Point& point, Scene* scene) : scene(scene) { data = point; }
    Light(const Spot& spot, Scene* scene) : scene(scene) { data = spot; }

    Light() : scene(nullptr) { data = Directional(); }
    ~Light();

    ELightType          getType() const { return ELightType(data.index()); }

    const Directional&  getDirectional() const { _ASSERTE(getType() == LIGHT_DIRECTIONAL); return std::get<Directional>(data); }
    const Point&        getPoint() const { _ASSERTE(getType() == LIGHT_POINT); return std::get<Point>(data); }
    const Spot&         getSpot() const { _ASSERTE(getType() == LIGHT_SPOT); return std::get<Spot>(data); }

    void setDirectional(const Directional& directional) {  data = directional; }
    void setPoint(const Point& point) { data = point; }
    void setSpot(const Spot& spot) { data = spot; } 

};