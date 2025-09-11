#pragma once

#include "Module.h"

#include <filesystem>
#include <vector>

namespace DirectX { class ScratchImage;  struct TexMetadata; }

// ModuleResources handles creation and management of GPU resources in DirectX 12.
// It provides functions to create buffers, textures, render targets, and depth stencils.
// The class manages temporary upload buffers and command lists for resource initialization.
// Note: Current implementation is not thread-safe and should only be used from a single thread.
class ModuleResources : public Module
{
private:

    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;

    // temporal data for loading textures (note: multithreading issues)
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
    std::vector<UINT> numRows;
    std::vector<UINT64> rowSizes;

    struct DeferredFree
    {
        UINT frame = 0;
        ComPtr<ID3D12Resource> resource;
    };

    std::vector<DeferredFree> deferredFrees;

public:

    ModuleResources();
    ~ModuleResources();

    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size, const char* name);
    ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);

    ComPtr<ID3D12Resource> createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format);
    ComPtr<ID3D12Resource> createTextureFromMemory(const void* data, size_t size, const char* name);
    ComPtr<ID3D12Resource> createTextureFromFile(const std::filesystem::path& path);

    ComPtr<ID3D12Resource> createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> createDepthStencil(DXGI_FORMAT format, size_t width, size_t height, float clearDepth, uint8_t clearStencil, const char* name);

    ComPtr<ID3D12Resource> createCubemapRenderTarget(DXGI_FORMAT format, size_t size, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> createCubemapRenderTarget(DXGI_FORMAT format, size_t size, size_t mipLevels, const Vector4& clearColour, const char* name);

    void deferRelease(ComPtr<ID3D12Resource> resource);

private:

    ComPtr<ID3D12Resource> createTextureFromImage(const ScratchImage& image, const char* name);
    ComPtr<ID3D12Resource> createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, size_t arraySize, size_t mipLevels, const Vector4& clearColour, const char* name);
    ComPtr<ID3D12Resource> getUploadHeap(size_t size);
};

inline ComPtr<ID3D12Resource> ModuleResources::createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, const Vector4& clearColour, const char* name)
{
    return createRenderTarget(format, width, height, 1, 1, clearColour, name);
}

inline ComPtr<ID3D12Resource> ModuleResources::createCubemapRenderTarget(DXGI_FORMAT format, size_t size, const Vector4& clearColour, const char *name)
{
    return createRenderTarget(format, size, size, 6, 1, clearColour, name);
}

inline ComPtr<ID3D12Resource> ModuleResources::createCubemapRenderTarget(DXGI_FORMAT format, size_t size, size_t mipLevels, const Vector4 &clearColour, const char *name)
{
    return createRenderTarget(format, size, size, 6, mipLevels, clearColour, name);
}
