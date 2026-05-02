#pragma once

class DecalCubeMesh
{
    struct Vertex { Vector3 position; };

   D3D12_INPUT_ELEMENT_DESC inputLayout;
   D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;

   ComPtr<ID3D12Resource> vertexBuffer;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
public:

    DecalCubeMesh();
    ~DecalCubeMesh();

    void draw(ID3D12GraphicsCommandList* commandList) const;

    const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }
};