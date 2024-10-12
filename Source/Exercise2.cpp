#include "Globals.h"
#include "Exercise2.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

static const char shaderSource[] = R"(
    cbuffer Transforms : register(b0)
    {
        float4x4 mvp;
    };

    float4 exercise2VS(float3 pos : POSITION) : SV_POSITION
    {
        return mul(float4(pos, 1.0f), mvp);
    }

    float4 exercise2PS() : SV_TARGET
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
)";

bool Exercise2::init() 
{
    struct Vertex
    {
        XMFLOAT3 position;
    };

    static Vertex vertices[3] = 
    {
        XMFLOAT3(-1.0f, -1.0f, 0.0f),  // 0
        XMFLOAT3(0.0f, 1.0f, 0.0f),    // 1
        XMFLOAT3(1.0f, -1.0f, 0.0f)    // 2
    };  
    
    bool ok = createVertexBuffer(&vertices[0], sizeof(vertices), sizeof(Vertex));
    ok = ok && createShaders();
    ok = ok && createRootSignature();
    ok = ok && createPSO();

    if (ok)
    {
        app->getD3D12()->signalDrawQueue();
    }

    return true;
}

void Exercise2::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList *commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), pso.Get());
    
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -20.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PI / 4.0f, float(width) / float(height), 0.1f, 1000.0f);

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
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);                   // set the vertex buffer (using the vertex buffer view)

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX)/sizeof(UINT32), &mvp, 0);

    commandList->DrawInstanced(3, 1, 0, 0);                                     // finally draw 3 vertices (draw the triangle)

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if(SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Exercise2::createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride)
{
    ModuleD3D12* d3d12  = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer)));
    
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    // TODO: use ring buffer for uploading resources
    ok = ok && SUCCEEDED(device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE,                             
        &bufferDesc,      
        D3D12_RESOURCE_STATE_GENERIC_READ,                
        nullptr,
        IID_PPV_ARGS(&bufferUploadHeap)));

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    ok = ok && SUCCEEDED(commandList->Reset(d3d12->getCommandAllocator(), nullptr));

    if (ok)
    {
        // Copy to intermediate
        BYTE* pData = nullptr;
        bufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        memcpy(pData, bufferData, bufferSize);
        bufferUploadHeap->Unmap(0, nullptr);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        // Copy to vram
        commandList->CopyBufferRegion(vertexBuffer.Get(), 0, bufferUploadHeap.Get(), 0, bufferSize);
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes  = stride;
        vertexBufferView.SizeInBytes    = bufferSize;
    }

    return ok;
}

bool Exercise2::createShaders()
{
    ComPtr<ID3DBlob> errorBuff;

#ifdef _DEBUG
    unsigned flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
    unsigned flags = 0;
#endif

    if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource), "exercise2VS", nullptr, nullptr, "exercise2VS", "vs_5_0", flags, 0, &vertexShader, &errorBuff)))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource), "exercise2PS", nullptr, nullptr, "exercise2PS", "ps_5_0", flags, 0, &pixelShader, &errorBuff)))
    {
        OutputDebugStringA((char*)errorBuff->GetBufferPointer());
        return false;
    }

    return true;
}

bool Exercise2::createRootSignature()
{
    // TODO: create root signature from HSLS

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[1];

    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(UINT32), 0);
    rootSignatureDesc.Init(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool Exercise2::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

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