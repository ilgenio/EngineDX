struct MetallicRoughnessMat
{
    float4 baseColour;
    float  metallicFactor;
    float  roughnessFactor;
    float  occlusionStrength;
    bool   hasBaseColourTex;
    bool   hasMetallicRoughnessTex;
    bool   hasOcclusionTex;
};

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerFrame : register(b1)
{
    float3 viewPos;             // Camera position
    float  roughnessLevels;
    bool   useOnlyIrradiance;
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    MetallicRoughnessMat material;
};


