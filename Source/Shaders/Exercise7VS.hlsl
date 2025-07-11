#include "Exercise7.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

struct VertexOutput
{
    float3 worldPos : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VertexOutput exercise7VS(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL)
{
    VertexOutput output;
    output.worldPos = mul(float4(position, 1.0), modelMat).xyz;
    output.normal   = mul(normal, (float3x3)normalMat);
    output.texCoord = texCoord;
    output.position = mul(float4(position, 1.0f), mvp);

    return output;
}

