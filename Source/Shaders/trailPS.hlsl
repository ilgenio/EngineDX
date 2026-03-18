#include "samplers.hlsli"

//Texture2D texture : register(t0);

float4 main(float2 texCoord : TEXCOORD, float3 color : COLOR) : SV_Target
{
#if 0
    float4 sampledColor = texture.Sample(bilinearWrap, texCoord);

    if(sampledColor.a < 0.1)
        discard;

    return  float4(sampledColor.rgb * color, 1.0);
#else
    return 1.0;
#endif     
}