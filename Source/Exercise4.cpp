#include "Globals.h"
#include "Exercise4.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"

#include "DirectXTex.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

static const char shaderSource[] = R"(
    cbuffer Transforms : register(b0)
    {
        float4x4 mvp;
    };

    struct VertexInput
    {
        float3 position : POSITION;
        float2 texCoord : TEXCOORD;
    };

    struct VertexOutput
    {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD;
    };

    VertexOutput exercise3VS(VertexInput input) 
    {
        VertexOutput output;
        output.position = mul(float4(input.position, 1.0f), mvp);
        output.texCoord = input.texCoord;

        return output;
    }

    Texture2D t1 : register(t1);
    SamplerState s1 : register(s0);

    float4 exercise3PS(VertexOutput input) : SV_TARGET
    {
        return t1.Sample(s1, input.texCoord);
    }
)";

bool Exercise4::init() 
{
    struct Vertex
    {
        Vector3 position;
        Vector2 uv;
    };

    static Vertex vertices[4] = 
    {
        { Vector3(-1.0f, -1.0f, 0.0f),  Vector2(0.0f, 1.0f) },
        { Vector3(-1.0f, 1.0f, 0.0f),   Vector2(0.0f, 0.0f) },
        { Vector3(1.0f, 1.0f, 0.0f),    Vector2(1.0f, 0.0f) },
        { Vector3(1.0f, -1.0f, 0.0f),   Vector2(1.0f, 1.0f) }
    };

    static short indices[6] = 
    {
        0, 1, 2,
        0, 2, 3
    };
    
    bool ok = createMainDescriptorHeap();
    ok = ok && createUploadFence();
    ok = ok && createVertexBuffer(&vertices[0], sizeof(vertices), sizeof(Vertex));
    ok = ok && createIndexBuffer(&indices[0], sizeof(indices));
    ok = ok && createShaders();
    ok = ok && createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadTextureFromFile(L"Lenna.tga", texture);
    ok = ok && loadTextureFromFile(L"dog.tga", textureDog);

    if (ok)
    {
        app->getD3D12()->signalDrawQueue();
    }

    return true;
}

bool Exercise4::cleanUp()
{
    if (uploadEvent) CloseHandle(uploadEvent);
    uploadEvent = NULL;

    return true;
}

void Exercise4::render()
{
    ModuleD3D12* d3d12  = app->getD3D12();
    ModuleCamera* camera = app->getCamera();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), pso.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    Matrix model = Matrix::Identity;
    const Matrix& view = camera->getView();
    const Matrix& proj = camera->getProj();

    mvp = model * view * proj;
    mvp = mvp.Transpose();

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width    = float(width); 
    viewport.Height   = float(height);

    D3D12_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = width;    
    scissor.bottom = height;

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);


    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);                   // set the vertex buffer (using the vertex buffer view)
    commandList->IASetIndexBuffer(&indexBufferView);                            // set the index buffer


    ID3D12DescriptorHeap *descriptorHeaps[] = {mainDescriptorHeap.Get()};
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix)/sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if(SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Exercise4::createBuffer(void* bufferData, unsigned bufferSize, ComPtr<ID3D12Resource>& buffer, D3D12_RESOURCE_STATES initialState)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    bool ok = SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&buffer)));

    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    ComPtr<ID3D12Resource> upload;

    ok = ok && SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload)));

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    ok = ok && SUCCEEDED(commandList->Reset(d3d12->getCommandAllocator(), nullptr));

    if (ok)
    {
        // Copy to intermediate
        BYTE* pData = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        upload->Map(0, &readRange, reinterpret_cast<void**>(&pData));
        memcpy(pData, bufferData, bufferSize);
        upload->Unmap(0, nullptr);

        // Copy to vram
        commandList->CopyResource(buffer.Get(), upload.Get());
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList };
        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadFenceCounter;
        queue->Signal(uploadFence.Get(), uploadFenceCounter);
        uploadFence->SetEventOnCompletion(uploadFenceCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);
    }

    return ok;
}

bool Exercise4::createIndexBuffer(void* bufferData, unsigned bufferSize)
{
    if(createBuffer(bufferData, bufferSize, indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER))
    {
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format         = DXGI_FORMAT_R16_UINT;
        indexBufferView.SizeInBytes    = bufferSize;

        return true;
    }

    return false;
}

bool Exercise4::createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride)
{
    if (createBuffer(bufferData, bufferSize, vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER))
    {
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes  = stride;
        vertexBufferView.SizeInBytes    = bufferSize;

        return true;
    }

    return false;
}

bool Exercise4::createShaders()
{
    ComPtr<ID3DBlob> errorBuff;

#ifdef _DEBUG
    unsigned flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
    unsigned flags = 0;
#endif

    if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource), "exercise3VS", nullptr, nullptr, "exercise3VS", "vs_5_0", flags, 0, &vertexShader, &errorBuff)))
    {
        LOG((char*)errorBuff->GetBufferPointer());
        return false;
    }

    if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource), "exercise3PS", nullptr, nullptr, "exercise3PS", "ps_5_0", flags, 0, &pixelShader, &errorBuff)))
    {
        LOG((char*)errorBuff->GetBufferPointer());
        return false;
    }

    return true;
}

bool Exercise4::createRootSignature()
{
    // TODO: create root signature from HSLS

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[2];

    rootParameters[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Constants.Num32BitValues = (sizeof(Matrix) / sizeof(UINT32));
    rootParameters[0].Constants.RegisterSpace  = 0;
    rootParameters[0].Constants.ShaderRegister = 0;

    D3D12_DESCRIPTOR_RANGE tableRange;

    tableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    tableRange.NumDescriptors = 2;
    tableRange.BaseShaderRegister = 0;
    tableRange.RegisterSpace = 0;
    tableRange.OffsetInDescriptorsFromTableStart = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &tableRange;

    D3D12_STATIC_SAMPLER_DESC sampler;
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    rootSignatureDesc.Init(2, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    ComPtr<ID3DBlob> rootSignatureBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;
}

bool Exercise4::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
    inputLayoutDesc.pInputElementDescs = inputLayout;

    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;                                  // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                           // the root signature that describes the input data this pso needs
    psoDesc.VS = vertexShaderBytecode;                                      // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = pixelShaderBytecode;                                       // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                     // format of the render target
    psoDesc.SampleDesc = {1, 0};                                            // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                        // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);       // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                 // a default blend state.
    psoDesc.NumRenderTargets = 1;                                           // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool Exercise4::createUploadFence()
{
    bool ok = SUCCEEDED(app->getD3D12()->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));

    if (ok)
    {
        uploadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        ok = uploadEvent != NULL;
    }

    return ok;
}

bool Exercise4::loadTextureFromFile(const wchar_t* fileName, ComPtr<ID3D12Resource>& texResource)
{
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRFile(fileName, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAFile(fileName, TGA_FLAGS_NONE, nullptr, image));
    
    ok = ok && loadTexture(image, texResource);    

    return ok;
}

bool Exercise4::loadTexture(const ScratchImage& image, ComPtr<ID3D12Resource>& texResource)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    const TexMetadata& metaData = image.GetMetadata();

    D3D12_RESOURCE_DESC desc = {};
    desc.Width            = static_cast<UINT>(metaData.width);
    desc.Height           = static_cast<UINT>(metaData.height);
    desc.MipLevels        = static_cast<UINT16>(metaData.mipLevels);
    desc.DepthOrArraySize = (metaData.dimension == TEX_DIMENSION_TEXTURE3D)
                            ? static_cast<UINT16>(metaData.depth)
                            : static_cast<UINT16>(metaData.arraySize);
    desc.Format           = metaData.format;
    desc.Flags            = D3D12_RESOURCE_FLAG_NONE;
    desc.SampleDesc.Count = 1;
    desc.Dimension        = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension);

    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    bool ok = SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texResource)));

    UINT64 requiredSize = 0;
    UINT64 rowSize      = 0;

    // \note: if mipmaps subresources are needed
    assert(desc.MipLevels == 1);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT  layout;

    // \note: upload buffer rows are aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
    device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, &rowSize, &requiredSize);

    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

    ComPtr<ID3D12Resource> textureUploadHeap;

    ok = ok && SUCCEEDED(device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE,                             
        &bufferDesc,      
        D3D12_RESOURCE_STATE_GENERIC_READ,                
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    ok = ok && SUCCEEDED(commandList->Reset(d3d12->getCommandAllocator(), nullptr));

    if (ok)
    {
        BYTE* uploadData = nullptr;
        textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));

        // \todo: if depth or array != 1 then another loop is needed
        for(size_t i=0; i< metaData.height; ++i)
        {
            memcpy(uploadData+i*layout.Footprint.RowPitch, image.GetPixels()+i*rowSize, rowSize);
        }

        CD3DX12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(texResource.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), layout);
        CD3DX12_RESOURCE_BARRIER barrier  = CD3DX12_RESOURCE_BARRIER::Transition(texResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        textureUploadHeap->Unmap(0, nullptr);
        commandList->CopyTextureRegion(&dst, 0, 0, 0, 
                                       &src, 
                                       nullptr);
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList };
        ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        ++uploadFenceCounter;
        queue->Signal(uploadFence.Get(), uploadFenceCounter);
        uploadFence->SetEventOnCompletion(uploadFenceCounter, uploadEvent);
        WaitForSingleObject(uploadEvent, INFINITE);
    }

    if(ok)
    {
        // now we create a shader resource view (descriptor that points to the texture and describes it)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                  = desc.Format;
        srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels     = 1;

        UINT srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle(mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.ptr += srvDescriptorSize * textureUploadHeaps.size();

        device->CreateShaderResourceView(texResource.Get(), &srvDesc, srvHandle);

        textureUploadHeaps.push_back(textureUploadHeap);
    }

    return ok;
}

bool Exercise4::createMainDescriptorHeap()
{ 
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    // create the descriptor heap that will store our srv
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    return SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap)));
}

