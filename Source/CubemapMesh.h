#pragma once


class CubemapMesh
{
   Vector3 front[6];
   Vector3 up[6];
   D3D12_INPUT_ELEMENT_DESC inputLayout;
   D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;

   ComPtr<ID3D12Resource> vertexBuffer;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

public:
    struct Vertex { Vector3 position; };
    enum Direction { POSITIVE_X = 0, NEGATIVE_X, POSITIVE_Y, NEGATIVE_Y, POSITIVE_Z, NEGATIVE_Z };

public:

    CubemapMesh();
    ~CubemapMesh();

#if 0
    constexpr uint32_t              getVertexCount() const { return 6 * 6; }
    ID3D12Resource*                 getVertexBuffer() const { return vertexBuffer.Get(); }
    const D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView() const { return vertexBufferView;  }
#endif

    void                            draw(ID3D12GraphicsCommandList* commandList) const;  
    const Vector3&                  getFrontDir(Direction dir) const { return front[uint32_t(dir)]; }
    const Vector3&                  getUpDir(Direction dir) const { return up[uint32_t(dir)]; }
    Matrix                          getViewMatrix(Direction dir) const {return Matrix::CreateLookAt(Vector3(0.0), front[dir], up[dir]);}

    // Input Layout Descriptor
    const D3D12_INPUT_LAYOUT_DESC& getInputLayoutDesc() { return inputLayoutDesc; }
};