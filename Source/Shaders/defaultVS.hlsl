#include "default.hlsli"
#include "global.hlsli"


PSInput main(in VSInput input)
{
    PSInput output;
    float4 worldPos    = mul(float4(input.position, 1.0), model);
    output.worldPos    = worldPos.xyz;
    output.texCoord    = input.texCoord;

    float3x3 normalMat = float3x3(normal[0].xyz, normal[1].xyz, normal[2].xyz);

    output.normal      = normalize(mul(input.normal, normalMat));
    output.tangent.xyz = normalize(mul(input.tangent.xyz, normalMat));
    output.tangent.w   = input.tangent.w;

	output.position    = mul(worldPos, viewProj);

    return output;
}

