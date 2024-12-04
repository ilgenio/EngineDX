#ifndef _TONEMAP_GLSL_
#define _TONEMAP_GLSL_

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


// From: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat = float3x3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);


// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat = float3x3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

// ACES filmic tone map approximation
float3 RRTAndODTFit(float3 color)
{
    float3 a = color * (color + 0.0245786) - 0.000090537;
    float3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}


// tone mapping 
float3 toneMapACES_Hill(float3 color)
{
    color = mul(color, ACESInputMat);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(color, ACESOutputMat);

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

float3 ACESFilm(in float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return lerp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}



float3 toneMap(float3 color)
{
    //color = ACESFilm(color);

    color /= 0.6;
    color = toneMapACES_Hill(color);


    return linearTosRGB(color);
}

#endif /* _TONEMAP_GLSL_ */