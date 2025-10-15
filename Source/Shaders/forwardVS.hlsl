#include "forward.hlsli"

struct VS_OUTPUT
{
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL0;
    centroid float3 centroidNormal : NORMAL1;
    float4 tangent  : TANGENT;
    float4 position : SV_POSITION;
};

VS_OUTPUT main(float3 position : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL, float4 tangent : TANGENT)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(position, 1.0), modelMat);
    output.position = mul(float4(position, 1.0), mvp);
    output.worldPos = worldPos.xyz;

    output.normal = normalize(mul(float4(normal, 0.0), normalMat)).xyz;
    output.centroidNormal = output.normal;
    output.tangent = float4(normalize(mul(float4(tangent.xyz, 0.0), normalMat)).xyz, tangent.w);
    output.texCoord = texCoord;

    return output;
}