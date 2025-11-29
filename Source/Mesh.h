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
    const BoundingBox& getBoundingBox() const { return bbox; }

    void draw(ID3D12GraphicsCommandList* commandList) const;

    // Input Layout Descriptor
    static const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }

private:

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void computeTSpace();
    void weld();

private:


    typedef std::unique_ptr<Vertex[]> VertexArray;
    typedef std::unique_ptr<uint8_t[]> IndexArray;

    // Name
    std::string name;

    // Vertex Data
    UINT numVertices = 0;
    UINT numIndices = 0;
    UINT indexElementSize = 0;

    VertexArray vertices;
    IndexArray indices;

    // Buffers
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    ComPtr<ID3D12Resource> skinningBuffer;
    BoundingBox bbox;

    static const uint32_t numVertexAttribs = 4;
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[numVertexAttribs]; 
    static const D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
};
