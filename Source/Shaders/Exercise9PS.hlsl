
TextureCube cubeMap : register(t0);
SamplerState cubeSampler : register(s0);

float4 exercise9PS(float3 texCoord : TEXCOORD) : SV_TARGET
{
    return cubeMap.Sample(cubeSampler, texCoord);
}
