cbuffer PerObject : register(b1)
{
    float4x4 modelMat;
    float3x3 normalMat;
    
    float4 diffuseColour;
    float Kd;
    float Ks;
    float shininess;
    bool hasDiffuseTex;
};

cbuffer Lighting : register(b2)
{
    float3 L;    // Light dir
    float3 Lc;   // Light colour
    float3 Ac;   // Ambient Colour
};

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

float4 exercise6PS(float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd   = hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour.rgb : diffuseColour.rgb;
    float3 N    = normalize(normal);
    float dotNL = saturate(-dot(L, N));

    return float4(Cd * Kd * dotNL, 1.0);
}

