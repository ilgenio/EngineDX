#version 460

#include "brdf.glsl"
#include "ibl.glsl"
#include "iridescence.glsl"
#include "material.glsl"
#include "globalMaps.glsl"
#include "normal.glsl"
#include "tonemap.glsl"

//output write
layout (location = 0) out vec4 outFragColor;

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoords;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec4 tangent;


vec3 Lighting(const vec3 N, const vec3 V, const vec3 L, const vec3 Lcolor, const vec3 albedo, const vec3 F0, float roughness, 
              float occlusion, float iridescenceFactor, float iridescenceThickness, float iridescenceIOR, float att)
{
    vec3 H      = normalize(-V-L);
    vec3 R      = reflect(V, N);

    float dotNL = max(dot(N, -L), 0.001);
    float dotNV = max(dot(N, -V), 0.001);

    float dotLH = max(dot(-L, H), 0.0);
    float dotNH = max(dot(H, N), 0.0001);

    // direct lighting 

    vec3 F      = F_Schlick(dotLH, F0);
    float D     = D_GGX(roughness, dotNH);
    float Vis   = smith_VGGX(dotNL, dotNV, roughness);

    if(iridescenceFactor != 0)
    {
        F = mix(F, F_Iridescence(dotLH, iridescenceIOR, iridescenceThickness, F0), iridescenceFactor);
    }

    vec3 colour =  ((albedo/PI)*(1-F)+(F*D*Vis))*Lcolor*dotNL*att; 

    // ibl (indirect) lighting
    if((global.debugFlags & DEBUG_DISABLE_IRIDESCENCE) != 0 || iridescenceFactor == 0.0)
    {
        colour += evaluateIBLGGX(albedo, F0, roughness, N, R, dotNV);  
    } 
    else 
    {
        vec3 iridescenceFr = F_Iridescence(dotNV, iridescenceIOR, iridescenceThickness, F0);

        if(global.debugRender == DEBUG_RENDER_IRIDESCENCE_FRESNEL)
        {
            colour = iridescenceFr;
        }
        else
        {
            colour += evaluateIBLIridescene(albedo, F0, iridescenceFr, iridescenceFactor, iridescenceIOR, roughness, N, R, dotNV);  // ibl (indirect) lighting
        }
    }

    return colour;
}

void main()
{
    Material material = getMaterial(texcoords);

    vec3 N = normal;
    if(material.hasNormal)
    {
        getNormal(material.normal, normal, tangent);
    }

    vec3 L = normalize(global.lightDir.xyz);
    vec3 V = normalize(position-global.viewPos.xyz);
    vec3 Lcolor = global.lightColor.rgb;

    vec3 colour = Lighting(N, V, L, Lcolor, material.albedo, material.F0, material.roughness, material.occlusion, 
                           material.iridescenceFactor, material.iridescenceThickness, material.iridescenceIOR, 1.0);
    
    if(global.debugRender == DEBUG_RENDER_NONE) outFragColor.rgb = toneMap(colour);
    else outFragColor.rgb = colour;

    outFragColor.a = material.alpha;
}
