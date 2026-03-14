#pragma once

#include <string>

namespace tinygltf { class Model;  struct Mesh; struct Primitive;  }

class Mesh
{
public:
    struct Vertex
    {
        Vector3 position;
        Vector2 texCoord0;
        Vector2 texCoord1;
        Vector3 normal;
        Vector3 tangent;
    };

public:

    Mesh();
    ~Mesh();

    void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    const std::string& getName() const {return name;}

    UINT getNumVertices() const {return numVertices; }
    UINT getNumIndices() const {return numIndices; }
    UINT getNumMorphTargets() const { return numMorphTargets; }
    const float* getInitialMorphWeights() const { return morphWeights.get();  }

    bool needsSkinning() const { return (boneData != 0) ; } 
    bool needsMorphing() const { return (numMorphTargets > 0); }

    D3D12_GPU_VIRTUAL_ADDRESS getBoneData() const { return boneData; }

    D3D12_GPU_VIRTUAL_ADDRESS getVertexBuffer() const { return vertexBufferView.BufferLocation; }
    D3D12_GPU_VIRTUAL_ADDRESS getIndexBuffer() const { return indexBufferView.BufferLocation; }
    D3D12_GPU_VIRTUAL_ADDRESS getMorphBuffer() const { return morphView.BufferLocation; }

    const D3D12_INDEX_BUFFER_VIEW& getIndexBufferView() const { return indexBufferView; }
    const D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView() const { return vertexBufferView; }
    const D3D12_VERTEX_BUFFER_VIEW& getMorphBufferView() const { return morphView; }

    const BoundingBox& getBoundingBox() const { return bbox; }

    // Input Layout Descriptor
    static const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }

private:

    struct SkinBoneData
    {
        UINT indices[4];
        Vector4 weights;
    };


    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void computeTSpace(std::unique_ptr<Vertex[]>& vertices, std::unique_ptr<uint8_t[]> &indices, std::unique_ptr<SkinBoneData[]>& bones);

private:

    // Name
    std::string name;

    // Vertex Data
    UINT numVertices = 0;
    UINT numIndices = 0;
    UINT indexElementSize = 0;
    UINT numMorphTargets = 0;

    // Buffers
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    D3D12_VERTEX_BUFFER_VIEW morphView;
    D3D12_GPU_VIRTUAL_ADDRESS boneData = 0;

    // bounding box
    BoundingBox bbox;

    std::unique_ptr<float[]> morphWeights; // initial morph target weights

    static const uint32_t numVertexAttribs = 5;
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[numVertexAttribs]; 
    static const D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
};

