#include "default.hlsli"
#include "global.hlsli"
#include "brdf.hlsli"
#include "material.hlsli"
#include "normal.hlsli"
#include "tonemap.hlsli"
#include "Iridescence.hlsli"

float3 Lighting(const float3 N, const float3 V, const float3 L, const float3 Lcolor, const float3 albedo, const float3 F0, float roughness, 
                float occlusion, float iridescenceFactor, float iridescenceThickness, float iridescenceIOR, float att)
{
    float3 H    = normalize(-V-L);
    float3 R    = reflect(V, N);

    float dotNL = max(dot(N, -L), 0.001);
    float dotNV = max(dot(N, -V), 0.001);

    float dotLH = max(dot(-L, H), 0.0);
    float dotNH = max(dot(H, N), 0.0001);

    // direct lighting 

    float3 F    = F_Schlick(dotLH, F0);
    float D     = D_GGX(roughness, dotNH);
    float Vis   = smith_VGGX(dotNL, dotNV, roughness);

    if(iridescenceFactor != 0)
    {
        F = lerp(F, F_Iridescence(dotLH, iridescenceIOR, iridescenceThickness, F0), iridescenceFactor);
    }

    float3 colour =  ((albedo/PI)*(1-F)+(F*D*Vis))*Lcolor*dotNL*att; 

    // ibl (indirect) lighting
    if((debugFlags & DEBUG_DISABLE_IRIDESCENCE) != 0 || iridescenceFactor == 0.0)
    {
        //colour += evaluateIBLGGX(albedo, F0, roughness, N, R, dotNV);  
    } 
    else 
    {
        float3 iridescenceFr = F_Iridescence(dotNV, iridescenceIOR, iridescenceThickness, F0);

        if(debugRender == DEBUG_RENDER_IRIDESCENCE_FRESNEL)
        {
            colour = iridescenceFr;
        }
        else
        {
            //colour += evaluateIBLIridescene(albedo, F0, iridescenceFr, iridescenceFactor, iridescenceIOR, roughness, N, R, dotNV);  // ibl (indirect) lighting
        }
    }

    return colour;
}

float4 main(in PSInput input) : SV_TARGET
{
    Material material = getMaterial(input.texCoord);

    float3 N;

    if(material.hasNormal)
    {
        N = getNormal(material.normal, input.normal, input.tangent);
    }
    else 
    {
        N = input.normal;
    }

    float3 L = normalize(lightDir.xyz);
    float3 V = normalize(input.position.xyz-viewPos.xyz);
    float3 Lcolor = lightColor.rgb;

    float3 colour = Lighting(N, V, L, Lcolor, material.albedo, material.F0, material.roughness, material.occlusion, 
                           material.iridescenceFactor, material.iridescenceThickness, material.iridescenceIOR, 1.0);
    
    if(debugRender == DEBUG_RENDER_NONE) 
    {
        colour = toneMap(colour);
    }

    return float4(colour, material.alpha);
}

