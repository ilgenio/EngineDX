#pragma once


#include "ShaderTableDesc.h"
#include "RenderTargetDesc.h"
#include "DepthStencilDesc.h"

// RenderTexture encapsulates a DirectX 12 render target and optional depth stencil.
// It manages creation, resizing, resource transitions, and descriptor handles for rendering and shader access.
// Use this class to render to textures and bind them as shader resources in your pipeline.
class RenderTexture
{
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> resolved;
    ComPtr<ID3D12Resource> depthTexture;

    UINT width = 0;
    UINT height = 0;

    DXGI_FORMAT format;
    DXGI_FORMAT depthFormat;
    const char* name;
    Vector4 clearColour;
    FLOAT clearDepth;
    bool autoResolveMSAA = false;
    bool msaa = false;

    ShaderTableDesc srvDesc;
    RenderTargetDesc rtvDesc;
    DepthStencilDesc dsvDesc;

public:

    RenderTexture(const char* name, DXGI_FORMAT format, const Vector4 clearColour, DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN, 
                  float clearDepth = 1.0f, bool msaa = false, bool autoResolve = false)
    : format(format), depthFormat(depthFormat), name(name), clearColour(clearColour), clearDepth(clearDepth), msaa(msaa), 
      autoResolveMSAA(autoResolve)
    { };

    ~RenderTexture();

    bool isValid() const { return width > 0 || height > 0;  }
    void resize(UINT width, UINT height);

    void beginRender(ID3D12GraphicsCommandList* cmdList)
    {
        //if (!msaa || !autoResolveMSAA)
        {
            transitionToRTV(cmdList);
        }

        setRenderTarget(cmdList);
    }

    void endRender(ID3D12GraphicsCommandList* cmdList)
    {
        if (msaa && autoResolveMSAA)
        {
            resolveMSAA(cmdList);
        }
        else
        {
            transitionToSRV(cmdList);
        }
    }

    UINT getWidth() const { return width;  }
    UINT getHeight() const { return height;  }
    D3D12_GPU_DESCRIPTOR_HANDLE getSrvHandle() const { return srvDesc.getGPUHandle(); }
    const ShaderTableDesc& getSrvTableDesc() const { return srvDesc;  }
    const RenderTargetDesc& getRtvDesc() const { return rtvDesc; }
    const DepthStencilDesc& getDsvDesc() const { return dsvDesc; }

private:
    void resolveMSAA(ID3D12GraphicsCommandList* cmdList);

    void transitionToRTV(ID3D12GraphicsCommandList* cmdList);
    void transitionToSRV(ID3D12GraphicsCommandList* cmdList);

    void setRenderTarget(ID3D12GraphicsCommandList* cmdList);
    void bindAsShaderResource(ID3D12GraphicsCommandList* cmdList, int slot);


};

