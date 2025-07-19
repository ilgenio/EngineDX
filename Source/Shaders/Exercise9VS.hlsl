
cbuffer MVP : register(b0)
{
    float4x4 vp;  
};

struct VertexOutput
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VertexOutput exercise9VS(float3 position : POSITION) 
{
    VertexOutput output;
    output.texCoord = position;
    output.position = mul(float4(position, 1.0), vp).xyww;

    return output;
}

