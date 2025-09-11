// Fullscreen Triangle Vertex Shader for DirectX 12
struct VSOutput
{
    float2 texcoord : TEXCOORD;
    float4 position : SV_Position;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    // Compute vertex position in clip space
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2);
    float2 clipPos = pos * float2(2, -2) + float2(-1, 1);

    VSOutput output;
    output.position = float4(clipPos, 0.0, 1.0);

    // Map to [0,1] for texture coordinates
    output.texcoord = pos;
    
    return output;
}