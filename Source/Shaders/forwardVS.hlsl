#include "forward.hlsli"

struct VS_OUTPUT
{
    float3 worldPos : POSITION0;
    float3 ndcPos   : POSITION1;
    float2 texCoord0 : TEXCOORD0;
    float2 texCoord1 : TEXCOORD1;
    float3 normal   : NORMAL0;
    float3 tangent  : TANGENT;
    float4 position : SV_POSITION;
};

VS_OUTPUT main(float3 position : POSITION, float2 texCoord0 : TEXCOORD0, float2 texCoord1 : TEXCOORD1, float3 normal : NORMAL, float3 tangent : TANGENT)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(position, 1.0), modelMat);
    output.position = mul(float4(position, 1.0), mvp);
    output.worldPos = worldPos.xyz;
    output.ndcPos = output.position.xyz / output.position.w;

    output.normal = normalize(mul(float4(normal, 0.0), normalMat)).xyz;
    output.tangent = normalize(mul(float4(tangent, 0.0), normalMat)).xyz;
    output.texCoord0 = texCoord0;
    output.texCoord1 = texCoord1;
    
    return output;
}