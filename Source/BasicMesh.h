#pragma once

#include <string>

namespace tinygltf { class Model;  struct Mesh; struct Primitive;  }

class BasicMesh
{
public:
    struct Vertex
    {
        Vector3 position = Vector3::Zero;
        Vector2 texCoord0 = Vector2::Zero;
        Vector3 normal = Vector3::UnitZ;
        Vector3 tangent = Vector3::UnitX;
    };

public:

    BasicMesh();
    ~BasicMesh();

    void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    const std::string& getName() const {return name;}

    uint32_t getNumVertices() const {return numVertices; }
    uint32_t getNumIndices() const {return numIndices; }

    void draw(ID3D12GraphicsCommandList* commandList) const;

    // Input Layout Descriptor
    static const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }

    int getMaterialIndex() const { return materialIndex; }

private:

    BasicMesh(const BasicMesh&) = delete;
    BasicMesh& operator=(const BasicMesh&) = delete;

    void clean();

private:


    typedef std::unique_ptr<Vertex[]> VertexArray;
    typedef std::unique_ptr<uint8_t[]> IndexArray;

    // Name
    std::string name;

    // Vertex Data
    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t indexElementSize = 0;
    int32_t  materialIndex = -1;

    VertexArray vertices;
    IndexArray indices;

    // Buffers
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    static const uint32_t numVertexAttribs = 4;
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[numVertexAttribs]; 
    static const D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
};
