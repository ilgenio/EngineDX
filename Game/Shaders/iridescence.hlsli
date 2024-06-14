#ifndef _IRIDESCENCE_GLSL_
#define _IRIDESCENCE_GLSL_

// Based on GLTF iridescence material implementation https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_iridescence/README.md#theory-documentation-and-implementations
// Basic Theory about thin film, constructive/destructive, waves interference https://en.wikipedia.org/wiki/Thin-film_interference
// and the original paper https://belcour.github.io/blog/research/publication/2017/05/01/brdf-thin-film.html

#include "common.hlsli"
#include "brdf.hlsli"

// XYZ to linear sRGB
// TODO: Constructor vs mat3
static const float3x3 XYZ_TO_sRGB = float3x3(
     3.2404542, -1.5371385, -0.4985314, 
    -0.9692660,  1.8760108, 0.0415560, 
     0.0556434, -0.2040259, 1.0572252
);

float3 F0ToIor(float3 fresnel0) 
{
    float3 sqrtF0 = sqrt(fresnel0);
    return (1.0 + sqrtF0) / (1.0 - sqrtF0);
}

float IorToF0(float transmittedIor, float incidentIor) 
{
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float3 IorToF0(float3 transmittedIor, float incidentIor) 
{
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

// Evaluation XYZ sensitivity curves in Fourier space
float3 evalSensitivity(float OPD, float3 shift) 
{
    float phase = 2.0 * PI * OPD * 1.0e-9;
    float3 val = float3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    float3 pos = float3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    float3 var = float3(4.3278e+09, 9.3046e+09, 6.6121e+09);

    float3 xyz = val * sqrt(2.0 * PI * var) * cos(pos * phase + shift) * exp(-sq(phase) * var);
    xyz.x += 9.7470e-14 * sqrt(2.0 * PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(-4.5282e+09 * sq(phase));
    xyz /= 1.0685e-7;

    // Note: Must be converted from XYZ to RGB because of result will be multiplied to RGB reflection intensities
    float3 rgb = mul(xyz, XYZ_TO_sRGB);
    return rgb;
}

float3 F_Iridescence(float dotLH, float filmIor, float filmThickness, float3 baseF0) 
{
    float3 I;

    // Force air IOR when thckness == 0
    filmIor = lerp(1.0, filmIor, smoothstep(0.0, 0.03, filmThickness));

    // Applying snells law we can compute costheta2, needed later to compute OPD (1.0 is air ior)
	float cosTheta2 = sqrt(1.0 - sq(1.0/filmIor)*(1-sq(dotLH)) ); 

    /////////////////////////
    // First interface
    /////////////////////////

    float filmF0 = IorToF0(filmIor, 1.0);     // Need F0 to compute Schlick Fresnel. Is a simplifcation over original paper that takes into account dielectric/conductor fresnels, a complex ior for conductors and light polarization
    float R12    = F_Schlick(dotLH, filmF0);  // Reflection film from air
    float R21 = R12;                          // Reflection film from film
    float T121 = 1.0 - R12;                   // Transmission film from air or from film
    float phi12 = 0.0;                        // Phase shift due to reflection in film from air
    if (filmIor < 1.0) phi12 = PI;            // If filmIor < air IOR then there is a phase shift of PI this is a simplification because this dependes on the polarizaation of the light but we are not going to take it into account

    float phi21 = PI - phi12;                 // Phase shift film from air is inverse of air to film 

    /////////////////////////
    // Second interface
    /////////////////////////

    // Compute IOR of the base material as we have F0 as linear sRGB colour
    // Computing IOR from F0 is not really possible because for metals the IOR is a complex number and is different for different frequency bands (colors) 
    // But we will use the conversion of each RGB component as an approximation

    // Trying to convert baseF0 to IOR for 2 reasons:
    //   * Use IOR to compute phase shift 
    //   * Convert back to baseF0 but original baseF0 was computed for air to base boundary and now it's computed using the film to base boundary
    float3 baseIOR = F0ToIor(clamp(baseF0, 0.0, 0.9999)); // guard against 1.0 (division by 0)

    // Phase shifts (equivalent to if baseIOR < filmIOR then PI else 0.0) 
    float3 phi23 = lerp(0.0, PI, baseIOR < filmIor);

    // Compute F0 from film and Apply Schlick Fresnel
    float3 R23 = F_Schlick(cosTheta2, IorToF0(baseIOR, filmIor)); // Reflection base from film 

    float OPD = 2.0 * filmIor * filmThickness * cosTheta2; // Optical Path Difference (the other phase shift)
    float3 phi = phi21 + phi23;                      // Phase Shifts film from air + base from film 

    /////////////////////////
    // Compound terms for the derivation
    // have a look at the original paper derivation https://belcour.github.io/blog/research/publication/2017/05/01/brdf-thin-film.html
    /////////////////////////

    float3 R21_23 = clamp(R21 * R23, 1e-5, 0.9999);  // R21*R23
    float3 r21_23 = sqrt(R21_23);
    float3 Rs = sq(T121) * R23 / (1.0 - R21_23); // Note: T12 is equal to T21 so sq(T121) is equivalent to T12*T21

    /////////////////////////
    // Reflectance term for m = 0 (DC term amplitude)
    /////////////////////////
    float3 C0 = R12 + Rs;
    I = C0;

    /////////////////////////
    // Reflectance term for m > 0 (pairs of diracs)
    /////////////////////////
    float3 Cm = Rs - T121; // Note: T12 is equal to T21 SO T121 is the square root of the multiplication

    for (int m = 1; m <= 2; ++m) // m = 2 is enough  due to testing
    {
        Cm *= r21_23;
        float3 Sm = 2.0 * evalSensitivity(float(m) * OPD, float(m) * phi);
        I += Cm * Sm;
    }

	return max(I, 0.0);
}


#endif /* _IRIDESCENCE_GLSL_ */