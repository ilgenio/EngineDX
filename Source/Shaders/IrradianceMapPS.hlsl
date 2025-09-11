#include "common.hlsli"

TextureCube environment : register(t0);
SamplerState samplerState : register(s0);

#define NUM_SAMPLES 1024

float3x3 computeTangetSpace(in float3 normal)
{
    float3 up    = abs(normal.y) > 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up           = cross(normal, right);
    
    return float3x3(right, up, normal);
}

float4 IrradianceMapPS(float3 texcoords : TEXCOORD) : SV_Target
{    
   float3 irradiance     = 0.0;
   float3 normal         = normalize(texcoords);
   float3x3 tangentSpace = computeTangetSpace(normal); // TBN matrix 

   for(int i=0; i< NUM_SAMPLES; ++i)
   {
       float2 rand_value = hammersley2D(i, NUM_SAMPLES);
       float3 L = mul(cosineSample(rand_value[0], rand_value[1]), tangentSpace);
       float3 Li = environment.Sample(samplerState, L).rgb;
       irradiance += Li;
   }  

   return float4(irradiance*(1.0/float(NUM_SAMPLES)), 1.0); 
}
