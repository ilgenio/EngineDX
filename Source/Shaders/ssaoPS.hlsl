 #include "samplers.hlsli"
 #include "common.hlsli"

 #define KERNEL_SIZE 16

 cbuffer SSAOParams : register(b0)
 {
    float4x4 proj;
    float4 kernel[KERNEL_SIZE];
    float kernelRadius;
    float bias;
    uint frameIndex;
    uint width;
    uint height;
    uint padding[3];
 };

Texture2D depthTexture : register(t0);
Texture2D normalTexture : register(t1);

float sampleIGN(float2 uv, float frameIndex)
{
    float2 pixelXY = uv * float2(width, height); 
    pixelXY += frameIndex * 5.588238;

    return frac(52.9829189f * frac(0.06711056 * float(pixelXY.x) + 0.00583715 * float(pixelXY.y)));
}

float3 getRandomTangent(float2 uv, uint frameIndex)
{
    float randomAngle = sampleIGN(uv, frameIndex) * 2.0 * PI; // Random angle in radians
    return float3(cos(randomAngle), sin(randomAngle), 0.0); // Random tangent in the XY plane
}

float3x3 computeTangentSpace(float3 normal, float3 randomTangent)
{
    // gram-schmidt orthogonalization
    float3 tangent = normalize(randomTangent - normal * dot(randomTangent, normal)); 
    float3 bitangent = cross(normal, tangent);
    return float3x3(tangent, bitangent, normal);
}

float4 main(in float2 texCoord : TEXCOORD) : SV_TARGET
{
    float depth = depthTexture.SampleLevel(pointClamp, texCoord, 0).r;
    float3 normal = normalTexture.SampleLevel(pointClamp, texCoord, 0).rgb;
    float3 viewPos = reconstructViewPosition(texCoord, depth, proj);
    float3x3 tangentSpace = computeTangentSpace(normal, getRandomTangent(texCoord, frameIndex));

    float occlusion = 0.0;

    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        float3 sampleViewPos = viewPos.xyz + mul(kernel[i].xyz, tangentSpace);

        // convert sample position from view space to clipping space
        float4 sampleNDCPos = mul(float4(sampleViewPos, 1.0), proj);

        // convert to NDC
        sampleNDCPos.xyz /= sampleNDCPos.w;

        // convert to UV coordinates for texture sampling
        float2 uv = ndcToUV(sampleNDCPos.xy);

        float sampleDepth = depthTexture.SampleLevel(pointClamp, uv, 0).r;

        float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(viewPos.z - lineariseDepth(sampleDepth, proj)));

        occlusion += ((sampleDepth+bias) >= sampleNDCPos.z ? 0.0 : 1.0) * rangeCheck;
    }

    return 1.0 - (occlusion / float(KERNEL_SIZE)); 
}