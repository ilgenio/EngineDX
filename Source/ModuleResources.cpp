#include "Globals.h"

#include "ModuleResources.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "DirectXTex.h"

namespace
{
    size_t alignUp(size_t value, size_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}

ModuleResources::ModuleResources()
{
}

ModuleResources::~ModuleResources()
{
}

bool ModuleResources::init()
{
    ModuleD3D12* d3d12   = app->getD3D12();
    ID3D12Device* device = d3d12->getDevice();

    bool ok SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Close());
    ok = ok && SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));

    uploadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok = uploadEvent != NULL;

    return true;
}

bool ModuleResources::cleanUp()
{
    if (uploadEvent) CloseHandle(uploadEvent);
    uploadEvent = NULL;

    return true;
}

ComPtr<ID3D12Resource> ModuleResources::createBuffer(void* data, size_t size, const char* name, size_t alignment)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    ComPtr<ID3D12Resource> buffer;

    size = alignUp(size, alignment); // NOTE: 256 Only for constant buffers
    
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer)));

    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    ID3D12Resource* upload = getUploadHeap(size);

    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    if (ok)
    {
        BYTE* pData = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        memcpy(pData, data, size);
        upload->Unmap(0, nullptr);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

        commandList->CopyBufferRegion(buffer.Get(), 0, upload, 0, size);
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList.Get()};
        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadCounter;
        queue->Signal(uploadFence.Get(), uploadCounter);
        uploadFence->SetEventOnCompletion(uploadCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);

        std::wstring convertStr(name, name + strlen(name));
        buffer->SetName(convertStr.c_str());
    }

    return buffer;
}

ComPtr<ID3D12Resource> ModuleResources::createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    D3D12_RESOURCE_DESC desc = {};
    desc.Width = UINT(width);
    desc.Height = UINT(height);
    desc.MipLevels = 1;
    desc.DepthOrArraySize = 1;
    desc.Format = format;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.SampleDesc.Count = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ComPtr<ID3D12Resource> texture = nullptr;

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));


    UINT64 requiredSize = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};

    device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, &requiredSize);

    ID3D12Resource* upload = getUploadHeap(requiredSize);

    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    if (ok)
    {
        BYTE* uploadData = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));
        for (uint32_t i = 0; i < height; ++i)
        {
            memcpy(uploadData + layout.Offset + layout.Footprint.RowPitch *i, (BYTE*)data + rowSize * i , rowSize);
        }

        upload->Unmap(0, nullptr);

        CD3DX12_TEXTURE_COPY_LOCATION Dst(texture.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION Src(upload, layout);
        commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList.Get() };
        ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadCounter;
        queue->Signal(uploadFence.Get(), uploadCounter);
        uploadFence->SetEventOnCompletion(uploadCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);
    }

    return texture;
}

ComPtr<ID3D12Resource> ModuleResources::createTextureFromMemory(const void* data, size_t size, const char* name)
{
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSMemory(data, size, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRMemory(data, size, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAMemory(data, size, TGA_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromWICMemory(data, size, DirectX::WIC_FLAGS_NONE, nullptr, image));

    // TODO: Check format support

    ok = ok && createTextureFromImage(image, name);

    return nullptr;
}

ComPtr<ID3D12Resource> ModuleResources::createTextureFromFile(const std::filesystem::path& path) 
{
    const wchar_t* fileName = path.c_str();
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRFile(fileName, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAFile(fileName, TGA_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromWICFile(fileName, DirectX::WIC_FLAGS_NONE, nullptr, image));

    // TODO: Check format support

    if (ok)
    {
        return createTextureFromImage(image, path.string().c_str());
    }

    return nullptr;
}

ComPtr<ID3D12Resource> ModuleResources::createTextureFromImage(const ScratchImage& image, const char* name)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    const TexMetadata& metaData = image.GetMetadata();

    D3D12_RESOURCE_DESC desc = {};
    desc.Width = UINT(metaData.width);
    desc.Height = UINT(metaData.height);
    desc.MipLevels = UINT16(metaData.mipLevels);
    desc.DepthOrArraySize = (metaData.dimension == TEX_DIMENSION_TEXTURE3D) ? UINT16(metaData.depth) : UINT16(metaData.arraySize);
    desc.Format = metaData.format;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.SampleDesc.Count = 1;
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension);

    ComPtr<ID3D12Resource> texture;

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));

    UINT64 requiredSize = 0;
    UINT numSubresources = UINT(metaData.mipLevels * metaData.arraySize);

    layouts.resize(numSubresources);
    numRows.resize(numSubresources);
    rowSizes.resize(numSubresources);

    device->GetCopyableFootprints(&desc, 0, numSubresources, 0, layouts.data(), numRows.data(), rowSizes.data(), &requiredSize);

    ID3D12Resource* upload = getUploadHeap(requiredSize);
        
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    if (ok)
    {
        BYTE* uploadData = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));

        for (UINT i = 0; i < numSubresources; ++i)
        {
            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[i];
            UINT rows = numRows[i];
            UINT64 rowSize = rowSizes[i];
            UINT64 depthSize = rowSize * rows;

            size_t mip = i % metaData.mipLevels;
            size_t item = i / metaData.mipLevels;

            for (UINT j = 0; j < layout.Footprint.Depth; ++j)
            {
                const Image* subImg = image.GetImage(mip, item, j);

                for (UINT k = 0; k < rows; ++k)
                {                    
                    memcpy(uploadData+layout.Offset + j * depthSize + k * rowSize, subImg->pixels + i * subImg->rowPitch, rowSize);
                }
            }
        }

        upload->Unmap(0, nullptr);

        for (UINT i = 0; i < numSubresources; ++i)
        {
            CD3DX12_TEXTURE_COPY_LOCATION Dst(texture.Get(), i);
            CD3DX12_TEXTURE_COPY_LOCATION Src(upload, layouts[i]);
            commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList.Get() };
        ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadCounter;
        queue->Signal(uploadFence.Get(), uploadCounter);
        uploadFence->SetEventOnCompletion(uploadCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);
    }

    if (ok)
    {
        texture->SetName(std::wstring(name, name+strlen(name)).c_str());
    }

    return texture;
}

ID3D12Resource* ModuleResources::getUploadHeap(size_t size)
{
    if (size > uploadSize)
    {
        ModuleD3D12* d3d12 = app->getD3D12();
        ID3D12Device* device = d3d12->getDevice();

        CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadHeap));
        uploadSize = size;
    }

    return uploadHeap.Get();
}

