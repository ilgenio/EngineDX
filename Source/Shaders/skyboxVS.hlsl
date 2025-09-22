
cbuffer VP : register(b0)
{
    float4x4 vp;  
    bool flipX;
    bool flipZ;
    uint padding[2];
};


struct VertexOutput
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VertexOutput skyboxVS(float3 position : POSITION) 
{
    VertexOutput output;
    
    output.texCoord = position;
    float4 clipPos = mul(float4(position, 1.0), vp);
    output.position = clipPos.xyww;
    
    if(flipX)
        output.texCoord.x = -output.texCoord.x;
    if (flipZ)    
        output.texCoord.z = -output.texCoord.z;

    return output;
}

