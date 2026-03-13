// HLSL skinning compute shader

#define SKINNING_GROUP_SIZE 64

cbuffer SkinConstants : register(b0)
{
    int numVertices;
}

cbuffer MorphConstants : register(b1)
{
    int numMorphTargets;
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

StructuredBuffer<float4x4> paletteModel : register(t0); 
StructuredBuffer<float4x4> paletteNormal : register(t1); 
StructuredBuffer<Vertex> inVertex : register(t2); 
StructuredBuffer<BoneWeight> boneWeights : register(t3); 

StructuredBuffer<float> morphWeights : register(t4);
StructuredBuffer<Vertex> morphVertices : register(t5);

RWStructuredBuffer<Vertex> outVertex : register(u0); 

Vertex morphVertex(uint index)
{
    Vertex vertex = inVertex[index];

    for (int i = 0; i < numMorphTargets; ++i)
    {        
        Vertex morphVertex = morphVertices[i * numVertices + index];
        
        vertex.position += morphVertex.position * morphWeights[i];
        vertex.normal   += morphVertex.normal * morphWeights[i];
        vertex.tangent  += morphVertex.tangent * morphWeights[i];
    }
    
    vertex.normal = normalize(vertex.normal);
    vertex.tangent = normalize(vertex.tangent);

    return vertex;
}


[numthreads(SKINNING_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID;
    
    if (index < numVertices)
    {
        // If there are morph targets, apply them
        // If not returns the original vertex
        Vertex vertex = morphVertex(index);
        
        BoneWeight bw = boneWeights[index];

        // Calculate skinning transform
        float4x4 skinModel = 
                paletteModel[bw.indices[0]] * bw.weights[0] +
                paletteModel[bw.indices[1]] * bw.weights[1] +
                paletteModel[bw.indices[2]] * bw.weights[2] +
                paletteModel[bw.indices[3]] * bw.weights[3];

        float4x4 skinNormal = 
                paletteNormal[bw.indices[0]] * bw.weights[0] +  
                paletteNormal[bw.indices[1]] * bw.weights[1] +
                paletteNormal[bw.indices[2]] * bw.weights[2] +
                paletteNormal[bw.indices[3]] * bw.weights[3];

        
        // Transform position and vectors
        vertex.position = mul(float4(vertex.position, 1.0), skinModel).xyz;
        vertex.normal   = mul(float4(vertex.normal, 0.0), skinNormal).xyz;
        vertex.tangent  = mul(float4(vertex.tangent, 0.0), skinNormal).xyz;

        // Store results
        outVertex[index]= vertex;
    }
}