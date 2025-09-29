#include "Globals.h"
#include "IrradianceMapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleRTDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleSamplers.h"
#include "CubemapMesh.h"
#include "ReadData.h"
#include "Math.h"

IrradianceMapPass::IrradianceMapPass()
{
    cubemapMesh = std::make_unique<CubemapMesh>();

    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device4* device = d3d12->getDevice();

    bool ok SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    ok = ok && createRootSignature();
    ok = ok && createPSO();

    _ASSERT_EXPR(ok, "IrradianceMapPass can't initialise");
}

IrradianceMapPass::~IrradianceMapPass()
{
}

ComPtr<ID3D12Resource> IrradianceMapPass::generate(D3D12_GPU_DESCRIPTOR_HANDLE cubemapSRV, size_t cubeSize, size_t irradianceSize)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleResources* resources = app->getResources();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();

    ComPtr<ID3D12Resource> irradianceMap = resources->createCubemapRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, irradianceSize, Vector4(0.0f, 0.0f, 0.0f, 1.0f), "Irradiance Map");

    BEGIN_EVENT(commandList.Get(), "Irradiance Map");

    // set necessary state
    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    // set viewport and scissor

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(irradianceSize), float(irradianceSize), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(irradianceSize), LONG(irradianceSize) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    commandList->SetGraphicsRootDescriptorTable(2, cubemapSRV);
    commandList->SetGraphicsRootDescriptorTable(3, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    // create render target view for each face
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(M_HALF_PI, 1.0f, 0.1f, 100.0f);

    Constants constants = {};
    constants.samples = 1024;
    constants.cubeMapSize = static_cast<INT>(cubeSize);
    constants.lodBias = 1.0;

    commandList->SetGraphicsRoot32BitConstants(1, sizeof(Constants) / sizeof(UINT32), &constants, 0);

    for (int i = 0; i < 6; ++i)
    {
        CubemapMesh::Direction dir = CubemapMesh::Direction(i);

        Matrix viewMatrix = cubemapMesh->getViewMatrix(dir);
        Matrix mvpMatrix = (viewMatrix * projMatrix).Transpose();

        SkyParams params = { mvpMatrix, cubemapMesh->flipX(dir), cubemapMesh->flipZ(dir) };

        commandList->SetGraphicsRoot32BitConstants(0, sizeof(SkyParams) / sizeof(UINT32), &params, 0);

        UINT subResource = D3D12CalcSubresource(0, i, 0, 1, 6);

        CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(irradianceMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResource);
        commandList->ResourceBarrier(1, &toRT);

        RenderTargetDesc rtDesc = rtDescriptors->create(irradianceMap.Get(), i, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtDesc.getCPUHandle();
        commandList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);

        cubemapMesh->draw(commandList.Get());

        CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(irradianceMap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResource);
        commandList->ResourceBarrier(1, &toSRV);
    }

    END_EVENT(commandList.Get());

    commandList->Close();

    ID3D12CommandList *commandLists[] = {commandList.Get()};
    d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    d3d12->flush();

    commandAllocator->Reset();
    SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    return irradianceMap;
}

bool IrradianceMapPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(SkyParams) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstants((sizeof(Constants) / sizeof(UINT32)), 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool IrradianceMapPass::createPSO()
{
    auto dataVS = DX::ReadData(L"skyboxVS.cso");
    auto dataPS = DX::ReadData(L"IrradianceMapPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = cubemapMesh->getInputLayoutDesc();
    psoDesc.pRootSignature = rootSignature.Get();                                                   
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;                                         
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc = {1, 0};                                                                    
    psoDesc.SampleMask = 0xffffffff;                                                                
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE; 
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         
    psoDesc.NumRenderTargets = 1;                                                                   

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}