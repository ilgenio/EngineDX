cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

struct VS_OUTPUT
{
    float3 ndcPos   : POSITION;
    float4 position : SV_POSITION;
};

VS_OUTPUT main(float3 position : POSITION)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(position, 1.0), mvp);
    output.position = worldPos;
    output.ndcPos   = worldPos.xyz / worldPos.w;

    return output;
}