cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

float4 main(float3 position : POSITION) : SV_POSITION
{
    return mul(float4(position, 1.0), mvp);
}