
cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

float4 exercise3VS(float3 pos : POSITION) : SV_POSITION
{
    return mul(float4(pos, 1.0f), mvp);
}

float4 exercise3PS() : SV_TARGET
{
    return t1.Sample(s1, float2(0.0, 0.0));
}
