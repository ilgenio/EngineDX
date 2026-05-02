#include "common.hlsli"
#include "samplers.hlsli"

cbuffer DecalConstants : register(b1)
{
    float4x4 projection;
    float4x4 invView;
    float4x4 invModel;
};

Texture2D depthMap : register(t0);
Texture2D baseColorMap : register(t1);
Texture2D normalMap : register(t3);

// Note: G-Buffer 
struct PSOutput
{
    float4 albedo : SV_Target0;
    float4 normalMetalRough : SV_Target1;
};

PSOutput main(float3 ndcPos : POSITION) 
{
    float2 uv = ndcToUV(ndcPos.xy);

    float depth      = depthMap.Sample(bilinearClamp, uv).r;
    float3 worldPos  = reconstructWorldPosition(uv, depth, projection, invView);
    float3 objectPos = mul(float4(worldPos, 1.0), invModel).xyz;

    if(abs(objectPos.x) > 0.5 || abs(objectPos.y) > 0.5 || abs(objectPos.z) > 0.5)
    {
        // Outside the decal volume, discard the pixel
        discard;
    }

    float2 objectUV = objectPos.xy + 0.5;
    objectUV.y = 1.0 - objectUV.y; // Flip Y coordinate for texture sampling

    PSOutput output;

    float4 baseColor = baseColorMap.Sample(bilinearWrap, objectUV);

    if(baseColor.a < 0.1)
    {
        // Transparent pixel, discard
        discard;
    }

    output.albedo = float4(baseColor.rgb, 1.0);
    output.normalMetalRough = 0.0;

    //float ao         = aoMap.Sample(bilinearWrap, objectUV).r;
    //float3 normal    = normalMap.Sample(bilinearWrap, objectUV).rgb;

    // TODO: Check G-Buffer packing  and ao and normal  and using normal maps 

    //output.normal = normal;
    //output.ao = ao;

    return output;
}
