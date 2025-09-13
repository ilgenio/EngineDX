struct MetallicRoughnessMat
{
    float4 baseColour;
    float  metallicFactor;
    float  roughnessFactor;
    bool   hasBaseColourTex;
    bool   hasMetallicRoughnessTex;
};

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    MetallicRoughnessMat material;
};


