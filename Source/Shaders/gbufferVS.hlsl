#include "gbuffer.hlsli"

struct VS_OUTPUT
{
    float3 worldPos : POSITION;
    float2 texCoord0 : TEXCOORD0;
    float2 texCoord1 : TEXCOORD1;
    float3 normal   : NORMAL0;
    float3 tangent  : TANGENT;
    float4 clipPos     : POSITION1;
    float4 clipPrevPos : POSITION2;
    float4 position : SV_POSITION;
};

struct Vertex
{
    float3 position;
    float2 texCoord0;
    float2 texCoord1;
    float3 normal;
    float3 tangent;
};

StructuredBuffer<Vertex> previousFrameVertices : register(t10);

VS_OUTPUT main(float3 position : POSITION, float2 texCoord0 : TEXCOORD0, float2 texCoord1 : TEXCOORD1, float3 normal : NORMAL, 
               float3 tangent : TANGENT, uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    float3 prevPos = position;

    if(isSkinned)
    {
        // Position from previous frame is fetched from the structured buffer containing the previous frame's vertex data
        prevPos = mul(float4(previousFrameVertices[vertexID].position, 1.0), modelMat).xyz;
    }

    float3 worldPos = mul(float4(position, 1.0), modelMat).xyz;
    output.position = mul(float4(position, 1.0), mvp);

    output.clipPos     = mul(float4(position, 1.0), mvpNoJitter);
    output.clipPrevPos = mul(float4(prevPos, 1.0), prevMVPNoJitter);

    output.worldPos  = worldPos;

    output.normal    = normalize(mul(float4(normal, 0.0), normalMat)).xyz;
    output.tangent   = normalize(mul(float4(tangent, 0.0), normalMat)).xyz;
    output.texCoord0 = texCoord0;
    output.texCoord1 = texCoord1;

    return output;
}