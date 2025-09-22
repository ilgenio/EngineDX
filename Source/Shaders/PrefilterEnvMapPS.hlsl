#include "ggx_brdf.hlsli"
#include "sampling.hlsli"
#include "ibl_common.hlsli"

cbuffer Constants : register(b2)
{
    float roughness;
    int numSamples;
    int cubemapSize;
    int lodBias;
};

TextureCube skybox : register(t0);
SamplerState skyboxSampler : register(s0);

float4 PrefilterEnvMapPS(float3 texcoords : TEXCOORD) : SV_Target
{
    float3 R = normalize(texcoords);
    float3 N = R, V = R;

    float3 color      = 0.0;
    float weight      = 0.0;
    float3x3 tangentSpace = computeTangetSpace(N);

    float alphaRoughness = roughness*roughness;

    alphaRoughness = max(alphaRoughness, 0.001); // avoid singularities

    for( int i = 0; i < numSamples; ++i ) 
    {
        float3 dir = ggxSample( hammersley2D(i, numSamples), alphaRoughness);

        // float pdf = D_GGX(roughness, NoH) * NoH / (4.0 * VoH);
        // but since V = N => VoH == NoH
        float pdf = D_GGX(alphaRoughness, dir.z) / 4.0;
        float lod = computeLod(pdf, numSamples, cubemapSize);

        float3 H = normalize(mul(dir, tangentSpace));
        float3 L = reflect(-V, H); 
        float NdotL = dot( N, L );
        if( NdotL > 0 ) 
        {
            color += skybox.SampleLevel(skyboxSampler, L, lod+lodBias).rgb * NdotL;
            weight += NdotL;
        }
    }

    return float4(color / weight, 1.0);
}
