#include "Globals.h"
#include "Exercise4.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"
#include "ModuleSamplers.h"

#include "ReadData.h"

#include "DirectXTex.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"

#include <imgui.h>

bool Exercise4::init() 
{
    struct Vertex
    {
        Vector3 position;
        Vector2 uv;
    };

    static Vertex vertices[4] = 
    {
        { Vector3(-1.0f, -1.0f, 0.0f),  Vector2(-0.2f, 1.2f) },
        { Vector3(-1.0f, 1.0f, 0.0f),   Vector2(-0.2f, -0.2f) },
        { Vector3(1.0f, 1.0f, 0.0f),    Vector2(1.2f, -0.2f) },
        { Vector3(1.0f, -1.0f, 0.0f),   Vector2(1.2f, 1.2f) }
    };

    static short indices[6] = 
    {
        0, 1, 2,
        0, 2, 3
    };
    
    bool ok = createUploadFence();
    ok = ok && createVertexBuffer(&vertices[0], sizeof(vertices), sizeof(Vertex));
    ok = ok && createIndexBuffer(&indices[0], sizeof(indices));
    ok = ok && createRootSignature();
    ok = ok && createPSO();

    if (ok)
    {
        ModuleResources* resources = app->getResources();

        textureDog = resources->createTextureFromFile(std::wstring(L"Assets/Textures/dog.dds"));

        ok = textureDog;
    }

    if(ok)
    {
        ModuleDescriptors* descriptors = app->getDescriptors();

        descriptors->allocateDescGroup(2, debugFonts);
        descriptors->allocateDescGroup(1, srvDog);
        descriptors->createTextureSRV(textureDog.Get(), srvDog);

        ModuleD3D12* d3d12 = app->getD3D12();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), debugFonts.getCPU(0), debugFonts.getGPU(0));

        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugFonts.getCPU(1), debugFonts.getGPU(1));
    }
     
    return true;
}

bool Exercise4::cleanUp()
{
    if (uploadEvent) CloseHandle(uploadEvent);
    uploadEvent = NULL;
    imguiPass.reset();

    return true;
}

void Exercise4::preRender()
{
    imguiPass->startFrame();
}

void Exercise4::render()
{
    ImGui::Begin("Texture Viewer Options");
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::Combo("Sampler", &sampler, "Linear/Wrap\0Point/Wrap\0Linear/Clamp\0Point/Clamp", SAMPLER_COUNT);
    ImGui::End();

    ModuleD3D12* d3d12  = app->getD3D12();
    ModuleCamera* camera = app->getCamera();
    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleSamplers* samplers = app->getSamplers();

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
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);                   // set the vertex buffer (using the vertex buffer view)
    commandList->IASetIndexBuffer(&indexBufferView);                            // set the index buffer


    ID3D12DescriptorHeap *descriptorHeaps[] = {descriptors->getHeap(), samplers->getHeap()};
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix)/sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootDescriptorTable(1, srvDog.getGPU(0));
    commandList->SetGraphicsRootDescriptorTable(2, samplers->getDefaultGroup().getGPU(sampler));

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    if(showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if(showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    debugDrawPass->record(commandList, width, height, view, proj);
    imguiPass->record(commandList);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if(SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Exercise4::createIndexBuffer(void* bufferData, unsigned bufferSize)
{
    ModuleResources* resources = app->getResources();
    indexBuffer = resources->createDefaultBuffer(bufferData, bufferSize, "QuadIB");

    if(indexBuffer)
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
    ModuleResources* resources = app->getResources();
    vertexBuffer = resources->createDefaultBuffer(bufferData, bufferSize, "QuadVB");

    if(vertexBuffer)
    {
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes  = stride;
        vertexBufferView.SizeInBytes    = bufferSize;

        return true;
    }

    return false;
}


bool Exercise4::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);   
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, app->getSamplers()->getDefaultGroup().getCount(), 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

    auto dataVS = DX::ReadData(L"Exercise4VS.cso");
    auto dataPS = DX::ReadData(L"Exercise4PS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };  // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = {1, 0};                                                                    // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

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

