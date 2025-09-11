#include "ggx_brdf.hlsli"
#include "sampling.hlsli"

cbuffer PrefilterEnvMapCB : register(b1)
{
    float roughness;
    int numSamples;
    int cubemapSize;
    int lodBias;
};

TextureCube skybox : register(t0);
SamplerState skyboxSampler : register(s0);

// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
float computeLod(float pdf, int numSamples, int width)
{
    // // note that 0.5 * log2 is equivalent to log4
    return max(0.5 * log2( 6.0 * float(width) * float(width) / (float(numSamples) * pdf)), 0.0);
}

float4 PrefilterEnvMapPS(float3 texcoords : TEXCOORD) : SV_Target
{
    float3 R = normalize(texcoords);
    float3 N = R, V = R;

    float3 color      = 0.0;
    float weight      = 0.0;
    float3x3 tangentSpace = computeTangetSpace(N);

    for( int i = 0; i < numSamples; ++i ) 
    {
        float3 dir = ggxSample( hammersley2D(i, numSamples), roughness);

        // float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
        // but since V = N => VoH == NoH
        float pdf = D_GGX(dir.z, roughness)/4.0;
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
