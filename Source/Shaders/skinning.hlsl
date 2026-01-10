// HLSL skinning compute shader

#define SKINNING_GROUP_SIZE 64

cbuffer SkinConstants : register(b0)
{
    int numVertices;
}

struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    float3 tangent;
};

StructuredBuffer<float4x4> palette : register(t0); 
Buffer<int4> boneIndices : register(t1); 
Buffer<float4> boneWeights : register(t2); 

StructuredBuffer<Vertex> inVertex : register(t3); 
RWStructuredBuffer<Vertex> outVertex : register(u0); 

[numthreads(SKINNING_GROUP_SIZE, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    
    if (index < numVertices)
    {
        Vertex vertex = inVertex[index];
        
        int4 indices = boneIndices[index];
        float4 weights = boneWeights[index];
        
        // Calculate skinning transform
        float4x4 skinTransform = 
            palette[indices[0]] * weights[0] +
            palette[indices[1]] * weights[1] +
            palette[indices[2]] * weights[2] +
            palette[indices[3]] * weights[3];
        
        // Transform position and vectors
        vertex.position = mul(float4(vertex.position, 1.0), skinTransform).xyz;
        vertex.normal = mul(float4(vertex.normal, 0.0), skinTransform).xyz;
        vertex.tangent = mul(float4(vertex.tangent, 0.0), skinTransform).xyz;

        // Store results
        outVertex[index] = vertex;
    }
}