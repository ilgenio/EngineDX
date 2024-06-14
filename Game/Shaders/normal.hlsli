#ifndef _NORMAL_GLSL_
#define _NORMAL_GLSL_


float3x3 createTBN(in float3 normal, in float4 tangent)
{
    float3 bitangent = tangent.w*cross(normal, tangent.xyz);

    return float3x3(normalize(tangent.xyz), normalize(bitangent), normalize(normal));
}

float3 getNormal(in float3 localNormal, in float3 normal, in float4 tangent)
{
    float3x3 tbn = createTBN(normal, tangent);
    return normalize(mul(localNormal, tbn));
}

#endif /* _NORMAL_GLSL_ */