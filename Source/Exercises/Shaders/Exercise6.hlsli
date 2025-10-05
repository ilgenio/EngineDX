
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
    
    float4 diffuseColour;
    float Kd;
    float Ks;
    float shininess;
    bool hasDiffuseTex;
};


