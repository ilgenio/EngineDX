#ifndef _MATERIAL_GLSL_
#define _MATERIAL_GLSL_

#include "brdf.hlsli"

// Maps
Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D occlusionTex : register(t3);
Texture2D iridescenceTex : register(t4);
Texture2D iridescenceThicknessTex : register(t5);

SamplerState defaultSampler : register(s0);

#define MATERIAL_FLAGS_NORMALS 1
#define MATERIAL_FLAGS_IRIDESCENCE 2
#define MATERIAL_FLAGS_IRIDESCENCE_TEX 4
#define MATERIAL_FLAGS_IRIDESCENCE_THICKNESS_TEX 8

struct Iridescence
{
    float factor;
    float IOR;
    float maxThikness;
    float minThikness;
};

// Material
cbuffer MaterialCB : register(b2)
{
	float4      baseColorFactor;
	float       metallicFactor;
	float       roughnessFactor;
    float       normalScale;
    int         flags;
    Iridescence iridescence;
    float       dielectricF0;
    int         matPad0;
    int         matPad1;
    int         matPad2;

};

struct Material
{
    float3      albedo;
    float       alpha;
    float3      F0;
    float       roughness;
    float       occlusion;

    bool        hasNormal;
    float3      normal;

    bool        hasIridescence;
    float       iridescenceFactor;
    float       iridescenceIOR;
    float       iridescenceThickness;
};


Material getMaterial(in float2 uv)
{
    Material material;

    float4 metRougSample   = metallicRoughnessTex.Sample(defaultSampler, uv);
    float4 baseColorSample = baseColorTex.Sample(defaultSampler, uv);

    float metallic    = clamp(metallicFactor*metRougSample.b, 0.0, 1.0);
    float roughness   = clamp(roughnessFactor*metRougSample.g, 0.0, 1.0);
    float3 baseColor  = baseColorSample.rgb*baseColorFactor.rgb;

    material.albedo         = lerp(baseColor, 0.0, metallic); 
    material.F0             = lerp(dielectricF0, baseColor, metallic); // TODO: change 0.04 for IOR extension
    material.roughness      = roughness*roughness;
    material.occlusion      = occlusionTex.Sample(defaultSampler, uv).r;
    material.alpha          = baseColorSample.a*baseColorFactor.a;
    material.normal         = normalize((normalTex.Sample(defaultSampler, uv).xyz * 2.0f - 1.0) * float3(normalScale, normalScale, 1.0));
    material.hasNormal      = (flags & MATERIAL_FLAGS_NORMALS) != 0 ? true : false;

    if((flags & MATERIAL_FLAGS_IRIDESCENCE) != 0 )
    {
        material.iridescenceFactor      = iridescence.factor;
        material.iridescenceIOR         = iridescence.IOR;
        material.iridescenceThickness   = iridescence.maxThikness;

        if((flags & MATERIAL_FLAGS_IRIDESCENCE_TEX) != 0)
        {
            material.iridescenceFactor *= iridescenceTex.Sample(defaultSampler, uv).r;
        }

        if((flags & MATERIAL_FLAGS_IRIDESCENCE_THICKNESS_TEX) != 0)
        {
            float thickness = iridescenceThicknessTex.Sample(defaultSampler, uv).g;
            material.iridescenceThickness = lerp(iridescence.minThikness, iridescence.maxThikness, thickness);
        }
    }
    else
    {
        material.iridescenceFactor = 0.0;
        material.iridescenceIOR    = 0.0;
        material.iridescenceThickness = 0.0;
    }

    return material;
}

#endif /* _MATERIAL_GLSL_ */