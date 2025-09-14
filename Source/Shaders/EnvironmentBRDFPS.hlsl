#include "ggx_brdf.hlsli"
#include "sampling.hlsli"

#define NUM_SAMPLES 1024

float4 EnvironmentBRDFPS(float2 uv : TEXCOORD) : SV_Target
{
    float NdotV = uv.x;
    float alphaRoughness = uv.y*uv.y;

    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV); // sin
    V.y = 0.0;
    V.z = NdotV; // cos

    float3 N = float3(0.0, 0.0, 1.0);

    precise float A = 0.0;
    precise float B = 0.0;

    for (uint i = 0; i < NUM_SAMPLES; i++) 
    {
        float3 H = ggxSample(hammersley2D(i, NUM_SAMPLES), alphaRoughness);

        // Get the light direction
        float3 L = reflect(-V, H); 

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0) 
        {
            float V_pdf = V_GGX(NdotL, NdotV, alphaRoughness) * 4 * VdotH * NdotL / NdotH;
            float Fc = pow(1.0 - VdotH, 5.0); // note: VdotH = LdotH
            A += (1.0 - Fc) * V_pdf;
            B += Fc * V_pdf;
        }
    }

    return float4(A/float(NUM_SAMPLES), B/float(NUM_SAMPLES), 0.0, 1.0);
}
