#ifndef _TONEMAP_HLSL_
#define _TONEMAP_HLSL_

static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / GAMMA;

// linear to sRGB approximation
float3 linearTosRGB(float3 color)
{
    return pow(color, INV_GAMMA);
}

// sRGB to linear approximation
float3 sRGBToLinear(float3 srgbIn)
{
    return float3(pow(srgbIn.xyz, GAMMA));
}

float4 sRGBToLinear(float4 srgbIn)
{
    return float4(sRGBToLinear(srgbIn.xyz), srgbIn.w);
}

float3 PBRNeutralToneMapping(float3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
        return color;

    const float d = 1. - startCompression;
    float newPeak = 1. - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return lerp(color, newPeak, g);
}

#endif /* _TONEMAP_HLSL_ */