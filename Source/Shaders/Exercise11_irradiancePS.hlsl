TextureCube cubemapTexture : register(t0);
SamplerState cubemapSampler : register(s0);

float4 Exercise11_irradiancePS(float3 positionWS : POSITION, float3 normalWS : NORMAL) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0); //    cubemapTexture.Sample(cubemapSampler, normalWS);
}