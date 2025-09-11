
cbuffer VP : register(b0)
{
    float4x4 vp;  
};

struct VertexOutput
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VertexOutput skyboxVS(float3 position : POSITION) 
{
    VertexOutput output;
    output.texCoord = position;
    float4 clipPos = mul(float4(position, 1.0), vp);
    output.position = clipPos.xyww;

    return output;
}

