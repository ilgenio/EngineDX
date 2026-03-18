cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

struct VS_OUTPUT
{
    float2 texCoord : TEXCOORD;
    float3 color  : COLOR;
    float4 position : SV_POSITION;
};

VS_OUTPUT main(float3 position : POSITION, float2 texCoord: TEXCOORD, float3 color : COLOR) 
{
    VS_OUTPUT output;

    output.position = mul(float4(position, 1.0), mvp);
    output.color    = color;
    output.texCoord = texCoord;

    return output;
}