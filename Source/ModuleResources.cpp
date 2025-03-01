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
    ID3D12Device4* device = d3d12->getDevice();

    bool ok SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    ok = ok && SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));

    uploadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok = uploadEvent != NULL;


    return ok;
}

bool ModuleResources::cleanUp()
{
    if (uploadEvent) CloseHandle(uploadEvent);
    uploadEvent = NULL;

    return true;
}

ComPtr<ID3D12Resource> ModuleResources::createUploadBuffer(void* data, size_t size, const char* name, size_t alignment)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    ComPtr<ID3D12Resource> buffer;

    size = alignUp(size, alignment);

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer));

    return buffer;
}


ComPtr<ID3D12Resource> ModuleResources::createDefaultBuffer(void* data, size_t size, const char* name, size_t alignment)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    ComPtr<ID3D12Resource> buffer;

    size = alignUp(size, alignment);

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(alignUp(size, alignment));
    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer)));

    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ID3D12Resource* upload = getUploadHeap(size);

    if (ok)
    {
        BYTE* pData = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        upload->Map(0, &readRange, reinterpret_cast<void**>(&pData));
        memcpy(pData, data, size);
        upload->Unmap(0, nullptr);

        commandList->CopyBufferRegion(buffer.Get(), 0, upload, 0, size);
        commandList->Close();
                          
        ID3D12CommandList* commandLists[] = { commandList.Get()};
        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadCounter;
        queue->Signal(uploadFence.Get(), uploadCounter);
        uploadFence->SetEventOnCompletion(uploadCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);

        commandAllocator->Reset();
        ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

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

    if (ok)
    {
        BYTE* uploadData = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        upload->Map(0, &readRange, reinterpret_cast<void**>(&uploadData));
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

        commandAllocator->Reset();
        ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));
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

    ComPtr<ID3D12Resource> texture;
    const TexMetadata& metaData = image.GetMetadata();

    _ASSERTE(metaData.dimension == TEX_DIMENSION_TEXTURE2D);

    if (metaData.dimension == TEX_DIMENSION_TEXTURE2D)
    {
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, UINT64(metaData.width), UINT(metaData.height),
            UINT16(metaData.arraySize), UINT16(metaData.mipLevels));

        CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
                                                            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));

        ID3D12Resource* upload = nullptr;
        if (ok)
        {
            _ASSERTE(metaData.mipLevels * metaData.arraySize == image.GetImageCount());
            upload = getUploadHeap(GetRequiredIntermediateSize(texture.Get(), 0, UINT(image.GetImageCount())));
            ok = upload != nullptr;
        }

        if (ok)
        {
            std::vector<D3D12_SUBRESOURCE_DATA> subData;
            subData.reserve(image.GetImageCount());

            for (size_t item = 0; item < metaData.arraySize; ++item)
            {
                for (size_t level = 0; level < metaData.mipLevels; ++level)
                {
                    const DirectX::Image* subImg = image.GetImage(level, item, 0);

                    D3D12_SUBRESOURCE_DATA data = { subImg->pixels, (LONG_PTR)subImg->rowPitch, (LONG_PTR)subImg->slicePitch };

                    subData.push_back(data);
                }
            }

            ok = UpdateSubresources(commandList.Get(), texture.Get(), upload, 0, 0, UINT(image.GetImageCount()), subData.data()) != 0;
        }

        if(ok)
        {
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

            commandAllocator->Reset();
            ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

            texture->SetName(std::wstring(name, name + strlen(name)).c_str());
            return texture;
        }
    }

    return ComPtr<ID3D12Resource>();
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

