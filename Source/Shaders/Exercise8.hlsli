static const float PI = 3.14159265f;

struct Directional
{
    float3 Ld;
    float  intensity;
    float3 Lc;
};

struct Point
{
    float3 Lp;
    float  sqRadius;
    float3 Lc;
    float  intensity;
};

struct Spot
{
    float3 Ld;
    float  sqRadius;
    float3 Lp;
    float  inner;
    float3 Lc;
    float  outter;
    float  intensity;
};

struct Ambient
{
    float3 Lc;
};

struct PhongMat
{
    float3 diffuseColour;
    bool hasDiffuseTex;
    float3 specularColour;
    float shininess;
};

StructuredBuffer<Point> pointLights : register(t0);
StructuredBuffer<Spot>  spotLights  : register(t1);

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerFrame : register(b1)
{
    Ambient ambient;       // Ambient Colour
    Directional dirLight;  // Directional light;
    uint numPointLights;   // Number of point lights 
    uint numSpotLights;    // Number of spot lights

    float3 viewPos;
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    PhongMat material;
};


