#pragma once

class RenderTexture
{
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> depthTexture;

    int width = 0;
    int height = 0;

    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
    const char* name;

    UINT rtvHandle = 0;
    UINT srvHandle = 0; 
    UINT dsvHandle = 0;

public:

    RenderTexture(DXGI_FORMAT format, const char* name, DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN) 
    : format(format), depthFormat(depthFormat), name(name) {};

    ~RenderTexture();

    void resize(int width, int height);

    void transitionToRTV(ID3D12GraphicsCommandList* commandList);
    void transitionToSRV(ID3D12GraphicsCommandList* commandList);

    void bindAsRenderTarget(ID3D12GraphicsCommandList* cmdList);
    void bindAsShaderResource(ID3D12GraphicsCommandList* cmdList, int slot);

    UINT getRTVHandle() const { return rtvHandle; }
    UINT getSRVHandle() const { return srvHandle; }
    UINT getDSVHandle() const { return dsvHandle; }
};