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

struct BoneWeight 
{
    int4 indices;
    float4 weights;
};

StructuredBuffer<float4x4> palette : register(t0); 
StructuredBuffer<Vertex> inVertex : register(t1); 
StructuredBuffer<BoneWeight> boneWeights : register(t2); 

RWStructuredBuffer<Vertex> outVertex : register(u0); 

[numthreads(SKINNING_GROUP_SIZE, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    
    if (index < numVertices)
    {
        Vertex vertex = inVertex[index];
        
        BoneWeight bw = boneWeights[index];
        
        // Calculate skinning transform
        float4x4 skinTransform = 
            palette[bw.indices[0]] * bw.weights[0] +
            palette[bw.indices[1]] * bw.weights[1] +
            palette[bw.indices[2]] * bw.weights[2] +
            palette[bw.indices[3]] * bw.weights[3];
        
        // Transform position and vectors
        vertex.position = mul(float4(vertex.position, 1.0), skinTransform).xyz;
        vertex.normal = mul(float4(vertex.normal, 0.0), skinTransform).xyz;
        vertex.tangent = mul(float4(vertex.tangent, 0.0), skinTransform).xyz;

        // Store results
        outVertex[index] = vertex;
    }
}