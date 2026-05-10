cbuffer Model : register(b0)
{
    float4x4 model;
};

cbuffer ViewProj : register(b1)
{
    float4x4 viewProj;
};

float4 main(float3 position : POSITION) : SV_POSITION
{
    return mul(float4(position, 1.0), mul(model, viewProj));
}