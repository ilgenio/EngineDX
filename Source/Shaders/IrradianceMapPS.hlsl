#include "common.hlsli"

TextureCube environment : register(t0);
SamplerState samplerState : register(s0);

#define NUM_SAMPLES 1024

float radicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley2D(uint i, uint N) 
{
    return float2(float(i)/float(N), radicalInverse_VdC(i));
}

float3x3 computeTangetSpace(in float3 normal)
{
    float3 up    = abs(normal.y) > 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up           = cross(normal, right);
    
    return float3x3(right, up, normal);
}

float3 cosineSample(float u1, float u2)
{
    float phi = 2.0 * PI * u1;
    float r = sqrt(u2);

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(1.0 - u2);

    return float3(x, y, z);
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
