
cbuffer Material : register(b1)
{
    float4 diffuseColour;
    float  Kd;
    float  Ks;
    float  shininess;
    bool   hasDiffuseTex;
};

cbuffer Lighting
{
    float3 L;               // Light dir
    float3 Lc;              // Light colour
    float3 ambientColour;   // Ambient Colour
};

cbuffer Camera : register(b2)
{
    float4x4 view;
    float3 cameraPos;
};

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

float4 exercise6PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd   = hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour.rgb : diffuseColour.rgb;
    float3 N    = normalize(normal);
    float dotNL = saturate(-dot(L, N));

    return float4(Cd * Kd * dotNL, 1.0);
}

