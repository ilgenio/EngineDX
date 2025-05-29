static const float PI = 3.14159265f;

cbuffer PerFrame : register(b1)
{
    float3 L; // Light dir
    float3 Lc; // Light colour
    float3 Ac; // Ambient Colour
    float3 viewPos;
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    float3 diffuseColour;
    bool hasDiffuseTex;
    float3 specularColour;
    float shininess;
};


