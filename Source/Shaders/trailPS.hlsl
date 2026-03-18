#include "samplers.hlsli"
#include "tonemap.hlsli"

Texture2D texture : register(t0);

float4 main(float2 texCoord : TEXCOORD, float3 color : COLOR) : SV_Target
{
    float3 output = texture.Sample(bilinearWrap, texCoord).rgb;
    
    output *= color;

    return float4(output, 1.0);
}
