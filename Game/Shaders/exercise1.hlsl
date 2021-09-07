float4 exercise1VS(float3 pos : POSITION) : SV_POSITION
{
    return float4(pos, 1.0);
}

float4 exercise1PS() : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
}