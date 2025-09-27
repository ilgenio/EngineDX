#include "common.hlsli"
#include "sampling.hlsli"
#include "ibl.hlsli"

cbuffer Constants : register(b2)
{
    int numSamples;
    int cubemapSize;
    int lodBias;
    int padding; // Padding to ensure 16-byte alignment
};

TextureCube environment : register(t0);
SamplerState samplerState : register(s0);

float4 IrradianceMapPS(float3 texcoords : TEXCOORD) : SV_Target
{    
   float3 irradiance     = 0.0;
   float3 normal         = normalize(texcoords);
   float3x3 tangentSpace = computeTangetSpace(normal); // TBN matrix 

   for(int i=0; i< numSamples; ++i)
   {
       float2 rand_value = hammersley2D(i, numSamples);
       float3 sample     = cosineSample(rand_value[0], rand_value[1]);
       float3 L          = mul(sample, tangentSpace);

       float pdf = sample.z/PI;
       float lod = computeLod(pdf, numSamples, cubemapSize);

       float3 Li = environment.SampleLevel(samplerState, L, lod + lodBias).rgb;

       irradiance += Li;
   }  

   return float4(irradiance*(1.0/float(numSamples)), 1.0);
}
