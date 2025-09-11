#pragma once

// RenderTexture encapsulates a DirectX 12 render target and optional depth stencil.
// It manages creation, resizing, resource transitions, and descriptor handles for rendering and shader access.
// Use this class to render to textures and bind them as shader resources in your pipeline.
class RenderTexture
{
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> depthTexture;

    int width = 0;
    int height = 0;

    DXGI_FORMAT format;
    DXGI_FORMAT depthFormat;
    const char* name;
    Vector4 clearColour;
    FLOAT clearDepth;

    UINT rtvHandle = 0;
    UINT srvHandle = 0; 
    UINT dsvHandle = 0;

public:

    RenderTexture(const char* name, DXGI_FORMAT format, const Vector4 clearColour, DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN, float clearDepth = 1.0f)
    : format(format), depthFormat(depthFormat), name(name), clearColour(clearColour), clearDepth(clearDepth) 
    { };

    ~RenderTexture();

    bool isValid() const { return width > 0 || height > 0;  }
    void resize(int width, int height);

    void transitionToRTV(ID3D12GraphicsCommandList* cmdList);
    void transitionToSRV(ID3D12GraphicsCommandList* cmdList);

    void bindAsRenderTarget(ID3D12GraphicsCommandList* cmdList);
    void clear(ID3D12GraphicsCommandList* cmdList);
    void bindAsShaderResource(ID3D12GraphicsCommandList* cmdList, int slot);

    UINT getRTVHandle() const { return rtvHandle; }
    UINT getSRVHandle() const { return srvHandle; }
    UINT getDSVHandle() const { return dsvHandle; }
};
