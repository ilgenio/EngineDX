cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};

struct VertexInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VertexOutput exercise3VS(VertexInput input) 
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.texCoord = input.texCoord;

    return output;
}

Texture2D t1 : register(t1);
SamplerState s1 : register(s0);

float4 exercise3PS(VertexOutput input) : SV_TARGET
{
    return t1.Sample(s1, input.texCoord);
}
