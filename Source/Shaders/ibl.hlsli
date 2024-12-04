#ifndef _IBL_GLSL_
#define _IBL_GLSL_

#include "globalMaps.glsl"
#include "globalUBO.glsl"
#include "brdf.glsl"
#include "iridescence.glsl"

vec3 evaluateIBLGGX(in vec3 albedo, in vec3 F0, in float roughness, in vec3 N, in vec3 R, in float dotNV)
{
    vec3 irradiance = textureLod(diffuseIBL, normalize(N), 0.0).rgb;
    vec3 radiance   = textureLod(specularIBL, R, roughness*(global.iblMipCount-1)).rgb;
    vec2 fab        = texture(brdfIBL, vec2(dotNV, roughness)).rg;

    return (albedo*(1-F0))*irradiance+radiance*(F0*fab.x+fab.y);
}

vec3 evaluateIBLIridescene(in vec3 albedo, in vec3 F0, in vec3 iridescenceFr, in float iridescenceFactor, 
                           in float iridescenceIOR, in float roughness, in vec3 N, in vec3 R, in float dotNV)
{
    vec3 irradiance    = textureLod(diffuseIBL, normalize(N), 0.0).rgb;

    vec3 iridescenceF0 = SchlickToF0(dotNV, iridescenceFr);
    vec3 Kd            = albedo*(1-max(iridescenceF0.r, max(iridescenceF0.g, iridescenceF0.b)));

    vec3 mixedF0 = mix(F0, iridescenceF0, iridescenceFactor);

    vec3 radiance   = textureLod(specularIBL, R, roughness*(global.iblMipCount-1)).rgb;
    vec2 fab        = texture(brdfIBL, clamp(vec2(dotNV, roughness), 0.0, 1.0)).rg;
    vec3 Ks         = (mixedF0*fab.x+fab.y);

    return Kd*irradiance+Ks*radiance;
}



#endif /* _IBL_GLSL_ */