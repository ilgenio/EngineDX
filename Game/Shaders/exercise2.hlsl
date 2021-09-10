
cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};

float4 exercise1VS(float3 pos : POSITION) : SV_POSITION
{
    return mul(float4(pos, 1.0f), mvp);
}

float4 exercise1PS() : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}