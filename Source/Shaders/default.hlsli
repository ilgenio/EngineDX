struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
};
  
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
};

cbuffer Tranforms : register(b1)
{
    float4x4 model;
    float4x4 normal;
};

