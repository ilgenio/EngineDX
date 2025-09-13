
TextureCube cubeMap : register(t0);
SamplerState cubeSampler : register(s0);

float4 skyboxPS(float3 texCoord : TEXCOORD) : SV_TARGET
{
    return cubeMap.Sample(cubeSampler, texCoord);
}
