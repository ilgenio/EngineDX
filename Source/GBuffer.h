#pragma once

#include "RenderTargetDesc.h"
#include "ShaderTableDesc.h"
#include "DepthStencilDesc.h"

// GBuffer: Manages the deferred-rendering geometry buffers (G-buffer).
// Holds albedo, normal/metallic/roughness and depth resources and
// provides resize/transition/render-target binding helpers.
class GBuffer
{
public:

    enum EBuffers
    {
        BUFFER_ALBEDO = 0,
        BUFFER_NORMAL_METALLIC_ROUGHNESS,
        BUFFER_EMISSIVE_AO, 
        BUFFER_COUNT
    };

private:

    ComPtr<ID3D12Resource> textures[BUFFER_COUNT];
    ComPtr<ID3D12Resource> depthTexture;
    
    RenderTargetDesc rtvDesc[BUFFER_COUNT];
    ShaderTableDesc  srvDesc;
    DepthStencilDesc dsvDesc;

    static const DXGI_FORMAT gBufferFormats[BUFFER_COUNT];
    static const const char* gBufferNames[BUFFER_COUNT];
    const DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

    UINT width  = 0;
    UINT height = 0;

public:

    GBuffer();
    ~GBuffer();

    bool isValid() const { return width > 0 || height > 0;  }
    void resize(UINT width, UINT height);

    void beginRender(ID3D12GraphicsCommandList* cmdList)
    {
        transitionToRTV(cmdList);
        setRenderTarget(cmdList);
    }

    void endRender(ID3D12GraphicsCommandList* cmdList)
    {
        transitionToSRV(cmdList);
    }

    UINT getWidth() const { return width;  }
    UINT getHeight() const { return height;  }

    const ShaderTableDesc& getSrvTableDesc() const { return srvDesc;  }
    const RenderTargetDesc& getRtvDesc(EBuffers buffer) const { return rtvDesc[int(buffer)]; }
    const DepthStencilDesc& getDsvDesc() const { return dsvDesc; }

    static const DXGI_FORMAT getRTFormat(UINT index) { return gBufferFormats[index];  }
    static UINT getRTFormatCount() const { return UINT(BUFFER_COUNT);  }
    static DXGI_FORMAT getDepthFormat() const { return depthFormat;  }

private:

    void transitionToRTV(ID3D12GraphicsCommandList* cmdList);
    void transitionToSRV(ID3D12GraphicsCommandList* cmdList);
    void setRenderTarget(ID3D12GraphicsCommandList* cmdList);
};

