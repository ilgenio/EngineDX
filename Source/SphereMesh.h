#pragma once

class SphereMesh
{
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
    D3D12_INPUT_ELEMENT_DESC inputLayout[3];
    uint32_t numIndices = 0;
    uint32_t numVertices = 0;
public:
    SphereMesh() = delete;
    SphereMesh(int slices, int stacks);
    ~SphereMesh();

    void draw(ID3D12GraphicsCommandList* commandList) const;

    const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }
};
