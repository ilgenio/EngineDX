#include "Globals.h"
#include "Exercise3.h"

#include "Application.h"
#include "ModuleRender.h"

#include "DirectXTex.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

bool Exercise3::init() 
{
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    static Vertex vertices[4] = 
    {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f),  XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0.0f),   XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 0.0f),    XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f),   XMFLOAT2(1.0f, 1.0f) }
    };

    static short indices[6] = 
    {
        0, 1, 2,
        0, 2, 3
    };
    
    bool ok = createMainDescriptorHeap();
    ok = ok && createVertexBuffer(&vertices[0], sizeof(vertices), sizeof(Vertex));
    ok = ok && createIndexBuffer(&indices[0], sizeof(indices));
    ok = ok && createShaders();
    ok = ok && createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadTextureFromFile(L"Lenna.tga");

    if (ok)
    {
        app->getRender()->signalDrawQueue();
    }

    return true;
}

UpdateStatus Exercise3::update()
{
    ModuleRender* render  = app->getRender();
    ID3D12GraphicsCommandList *commandList = render->getCommandList();

    commandList->Reset(render->getCommandAllocator(), pso.Get());
    
    unsigned width, height;
    app->getRender()->getWindowSize(width, height);

    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX view  = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX proj  = XMMatrixPerspectiveFovLH(XM_PI / 4.0f, float(width) / float(height), 0.1f, 1000.0f);

    XMStoreFloat4x4(&mvp, XMMatrixTranspose(model * view * proj));

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
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = render->getRenderTarget();

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

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX)/sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    if(SUCCEEDED(commandList->Close()))
    {
        render->executeCommandList();
    }

    return UPDATE_CONTINUE;
}

bool Exercise3::createBuffer(void *bufferData, unsigned bufferSize, ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& upload)
{
    ModuleRender* render  = app->getRender();
    ID3D12Device2* device = render->getDevice();

    bool ok = SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COPY_DEST, 
        nullptr, 
        IID_PPV_ARGS(&buffer)));

    ok = ok && SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
        D3D12_HEAP_FLAG_NONE,                             
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),      
        D3D12_RESOURCE_STATE_GENERIC_READ,                
        nullptr,
        IID_PPV_ARGS(&upload)));

    ID3D12GraphicsCommandList* commandList = render->getCommandList();
    ok = ok && SUCCEEDED(commandList->Reset(render->getCommandAllocator(), nullptr));

    if (ok)
    {
        // Copy to intermediate
        BYTE* pData = nullptr;
        upload->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        memcpy(pData, bufferData, bufferSize);
        upload->Unmap(0, nullptr);

        // Copy to vram
        commandList->CopyBufferRegion(buffer.Get(), 0, upload.Get(), 0, bufferSize);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
        commandList->Close();

        render->executeCommandList();
    }

    return ok;
}

bool Exercise3::createIndexBuffer(void* bufferData, unsigned bufferSize)
{
    if(createBuffer(bufferData, bufferSize, indexBuffer, iBufferUploadHeap))
    {
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format         = DXGI_FORMAT_R16_UINT;
        indexBufferView.SizeInBytes    = bufferSize;

        return true;
    }

    return false;
}

bool Exercise3::createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride)
{
    if(createBuffer(bufferData, bufferSize, vertexBuffer, vBufferUploadHeap))
    {
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes  = stride;
        vertexBufferView.SizeInBytes    = bufferSize;

        return true;
    }

    return false;
}

bool Exercise3::createShaders()
{
    ComPtr<ID3DBlob> errorBuff;

#ifdef _DEBUG
    unsigned flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
    unsigned flags = 0;
#endif

    if (FAILED(D3DCompileFromFile(L"Shaders/Exercise3.hlsl", nullptr, nullptr, "exercise3VS", "vs_5_0", flags, 0, &vertexShader, &errorBuff)))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    if (FAILED(D3DCompileFromFile(L"Shaders/Exercise3.hlsl", nullptr, nullptr, "exercise3PS", "ps_5_0", flags, 0, &pixelShader, &errorBuff)))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    return true;
}

bool Exercise3::createRootSignature()
{
    // TODO: create root signature from HSLS

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[2];

    rootParameters[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Constants.Num32BitValues = sizeof(XMMATRIX) / sizeof(UINT32);
    rootParameters[0].Constants.RegisterSpace  = 0;
    rootParameters[0].Constants.ShaderRegister = 0;

    D3D12_DESCRIPTOR_RANGE tableRange;

    tableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    tableRange.NumDescriptors = 1;
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

    if (FAILED(app->getRender()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;
}

bool Exercise3::createPSO()
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
    return SUCCEEDED(app->getRender()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool Exercise3::loadTextureFromFile(const wchar_t* fileName) 
{
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRFile(fileName, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAFile(fileName, TGA_FLAGS_NONE, nullptr, image));
    
    ok = ok && loadTexture(image);

    return ok;
}

bool Exercise3::loadTexture(const ScratchImage& image)
{
    ModuleRender* render  = app->getRender();
    ID3D12Device2* device = render->getDevice();

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

    bool ok = SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)));

    UINT64 requiredSize = 0;
    UINT64 rowSize      = 0;

    // \note: if mipmaps subresources are needed
    assert(desc.MipLevels == 1);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;

    // \note: upload buffer rows are aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
    device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, &rowSize, &requiredSize);

    ok = ok && SUCCEEDED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
        D3D12_HEAP_FLAG_NONE,                             
        &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),      
        D3D12_RESOURCE_STATE_GENERIC_READ,                
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    ID3D12GraphicsCommandList* commandList = render->getCommandList();
    ok = ok && SUCCEEDED(commandList->Reset(render->getCommandAllocator(), nullptr));

    if (ok)
    {
        BYTE* uploadData = nullptr;
        textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&uploadData));

        // \todo: if depth or array != 1 then another loop is needed
        for(size_t i=0; i< metaData.height; ++i)
        {
            memcpy(uploadData+i*layout.Footprint.RowPitch, image.GetPixels()+i*rowSize, rowSize);
        }

        textureUploadHeap->Unmap(0, nullptr);
        commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(texture.Get(), 0), 0, 0, 0, 
                                       &CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), layout), 
                                       nullptr);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        commandList->Close();
        render->executeCommandList();
    }

    if(ok)
    {
        // now we create a shader resource view (descriptor that points to the texture and describes it)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                  = desc.Format;
        srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels     = 1;
        device->CreateShaderResourceView(texture.Get(), &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    return ok;
}

bool Exercise3::createMainDescriptorHeap()
{ 
    ModuleRender* render = app->getRender();
    ID3D12Device2* device = render->getDevice();

    // create the descriptor heap that will store our srv
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    return SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap)));
}