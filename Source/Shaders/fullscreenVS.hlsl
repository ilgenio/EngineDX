static const float2 positions[3] = { float2(-1.0, 1.0), float2(3.0, 1.0), float2(-1.0, -3.0) };
static const float2 uvs[3] = { float2(0.0, 0.0), float2(2.0, 0.0), float2(0.0, 2.0) };

void main(uint vertexID : SV_VertexID, out float2 texcoord : TEXCOORD, out float4 position : SV_Position)
{  
    texcoord = uvs[vertexID];
    position = float4(positions[vertexID], 0.0, 1.0);
}   