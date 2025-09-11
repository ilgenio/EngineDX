#include "ggx_brdf.hlsli"

#define NUM_SAMPLES 1024

float4 EnvironmentBRDFPS(float2 uv : TEXCOORD) : SV_Target
{
    float NdotV = uv.x;
    float roughness = uv.y;

    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV); // sin
    V.y = 0.0;
    V.z = NdotV; // cos

    float3 N = vec3(0.0, 0.0, 1.0);

    float fa = 0.0;
    float fb = 0.0;

    for (uint i = 0; i < NUM_SAMPLES; i++) 
    {
        float3 H = ggxSample(hammersley2D(i, NUM_SAMPLES), roughness);

        // Get the light direction
        float3 L = reflect(-V, H); 

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0) 
        {
            float V_pdf = V_GGX(NdotL, NdotV, roughness) * VdotH * NdotL / NdotH;
            float Fc = pow(1.0 - VdotH, 5.0); // note: VdotH = LdotH
            fa += (1.0 - Fc) * V_pdf;
            fb += Fc * V_pdf;
        }
    }

    fragColor = float4(4.0*fa/float(NUM_SAMPLES), 4.0*fb/float(NUM_SAMPLES), 1.0, 1.0);
}
