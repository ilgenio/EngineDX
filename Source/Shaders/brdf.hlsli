#ifndef _BRDF_GLSL_
#define _BRDF_GLSL_

#include "common.hlsli"

float F_Schlick(in float cosTheta, in float f0)
{
    float x  = 1.0-cosTheta;
    float x2 = x*x;
    float x5 = x2*x2*x;

    return f0+(1-f0)*x5;
}

float3 F_Schlick(in float cosTheta, in float3 f0)
{
    float x   = 1.0-cosTheta;
    float x2  = x*x;
    float3 x5 = x2*x2*x;

    return f0+(1-f0)*x5;
}

float3 SchlickToF0(in float cosTheta, in float3 Fr)
{
    float x   = 1.0-cosTheta;
    float x2  = x*x;
    float3 x5 = x2*x2*x;

    return (Fr-x5)/(1-x5);
}

// from filament
float D_GGX(in float roughness, in float dotNH)
{
    // TODO: Check differences from  https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/brdf.glsl
    /*
    float a = NdotH*roughness;
    float k = roughness/max(1.0-NdotH*NdotH+a*a, 0.001);
    return k*k*(1.0/PI);
    */

    float roughnessSq = roughness * roughness;
    float f = max((dotNH * dotNH) * (roughnessSq - 1.0) + 1.0, 0.001);
    return roughnessSq / (PI * f * f);
}

float smith_VGGX(in float NdotL, in float NdotV, in float roughness)
{
    // TODO: Check differences from  https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/brdf.glsl
    // optimized version
    //return 0.5/mix(2.0*NdotL*NdotV, NdotL+NdotV, roughness);

    float roughnessSq = roughness * roughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - roughnessSq) + roughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - roughnessSq) + roughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }

    return 0.0;
}

float3 GGX_BRDF(in float3 N, in float3 V, in float3 L, in float3 Lcolor, in float3 F0, in float roughness, in float att)
{
    float3 H       = normalize(-V-L);

    float dotNL    = max(dot(N, -L), 0.001);
    float dotNV    = max(dot(N, -V), 0.001);

    float dotLH    = max(dot(-L, H), 0.0);
    float dotNH    = max(dot(H, N), 0.0001);

    float3 F       = F_Schlick(dotLH, F0);
    float D        = D_GGX(roughness, dotNH);
    float Vis      = smith_VGGX(dotNL, dotNV, roughness);

    // TODO: Check differences from  https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/brdf.glsl
    // 0.25 is optimized version of VSF

    return /*0.25**/F*D*Vis*Lcolor*dotNL*att;
}

float3 Lambert_GGX_BRDF(in float3 N, in float3 V, in float3 L, in float3 Lcolor, in float3 albedo, in float3 F0, in float roughness, in float att)
{
    float3 H       = normalize(-V-L);

    float dotNL    = max(dot(N, -L), 0.001);
    float dotNV    = max(dot(N, -V), 0.001);

    float dotLH    = max(dot(-L, H), 0.0);
    float dotNH    = max(dot(H, N), 0.0001);

    float3 F       = F_Schlick(dotLH, F0);
    float D        = D_GGX(roughness, max(0.001, dot(H, N)));
    float Vis      = smith_VGGX(dotNL, dotNV, roughness);

    // TODO: Check differences from  https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/brdf.glsl
    // 0.25 is optimized version of VSF

    return ((albedo/PI)*(1-F0)+(/*0.25**/F*D*Vis))*Lcolor*dotNL*att;
}

#endif /* _BRDF_GLSL_ */