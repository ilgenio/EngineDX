Texture2D SrcMipLevel : register(t0);
SamplerState bilinearSampler : register(s0);

float4 mipChainPS(float2 uv : TEXCOORD) : SV_Target
{
    return SrcMipLevel.Sample(bilinearSampler, uv);
}