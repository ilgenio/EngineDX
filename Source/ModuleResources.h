#pragma once

#include "Module.h"

#include <filesystem>
#include <vector>

namespace DirectX { class ScratchImage;  struct TexMetadata; }

class ModuleResources : public Module
{
private:

    uint32_t uploadCounter = 0;
    HANDLE uploadEvent = NULL;
    ComPtr<ID3D12Fence1> uploadFence;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12Resource> uploadHeap;
    size_t uploadSize = 0;

    // temporal data for loading textures (note: multithreading issues)
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
    std::vector<UINT> numRows;
    std::vector<UINT64> rowSizes;

public:

    ModuleResources();
    ~ModuleResources();

    bool init() override;
    bool cleanUp() override;

    ID3D12Resource* createBuffer(void* data, size_t size, const char* name);

    ID3D12Resource* createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format);
    ID3D12Resource* createTextureFromMemory(const void* data, size_t size, const char* name);
    ID3D12Resource* createTextureFromFile(const std::filesystem::path& path);

private:

    ID3D12Resource* createTextureFromImage(const ScratchImage& image, const char* name);
    ID3D12Resource* getUploadHeap(size_t size);
};

