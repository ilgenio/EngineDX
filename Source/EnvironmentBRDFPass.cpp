#include "Globals.h"

#include "EnvironmentBRDFPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleRTDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleSamplers.h"

#include "ReadData.h"

EnvironmentBRDFPass::EnvironmentBRDFPass()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device4* device = d3d12->getDevice();

    bool ok SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    ok = ok && createRootSignature();
    ok = ok && createPSO();

    _ASSERT_EXPR(ok, "Error creating EnvironmentBRDFPass");
}

EnvironmentBRDFPass::~EnvironmentBRDFPass()
{
}

ComPtr<ID3D12Resource> EnvironmentBRDFPass::generate(size_t size)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleResources* resources = app->getResources();

    ComPtr<ID3D12Resource> environmentMap = resources->createRenderTarget(DXGI_FORMAT_R16G16_FLOAT, size, size, Vector4(0.0f, 0.0f, 0.0f, 1.0f), "EnvironmentBRDF Map");

    BEGIN_EVENT(commandList.Get(), "EnvironmentBRDF Map");

    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(size), float(size), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(size), LONG(size) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    RenderTargetDesc rtvDesc = rtDescriptors->create(environmentMap.Get());
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtvDesc.getCPUHandle();
    commandList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);

    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(environmentMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRT);

    // Draw fullscreen triangle
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(environmentMap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);

    END_EVENT(commandList.Get());

    commandList->Close();
    ID3D12CommandList *commandLists[] = {commandList.Get()};
    d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    d3d12->flush();

    commandAllocator->Reset();
    SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    return environmentMap;
}

bool EnvironmentBRDFPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob)))
    {
        std::wstring msg((char*)errorBlob->GetBufferPointer(), (char*)errorBlob->GetBufferPointer() + errorBlob->GetBufferSize());
        _ASSERT_EXPR(false, msg.c_str());

        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;
}

bool EnvironmentBRDFPass::createPSO()
{
    auto dataVS = DX::ReadData(L"fullscreenVS.cso");
    auto dataPS = DX::ReadData(L"EnvironmentBRDFPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0};
    psoDesc.pRootSignature = rootSignature.Get();                                                   
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;                                         
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc = {1, 0};                                                                    
    psoDesc.SampleMask = 0xffffffff;                                                                
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         
    psoDesc.NumRenderTargets = 1;                                                                   

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}