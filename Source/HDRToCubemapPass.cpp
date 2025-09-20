#include "Globals.h"

#include "HDRToCubemapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "SingleDescriptors.h"
#include "TableDescriptors.h"
#include "ModuleRTDescriptors.h"

#include "CubemapMesh.h"
#include "ReadData.h"

HDRToCubemapPass::HDRToCubemapPass()
{
    cubemapMesh = std::make_unique<CubemapMesh>();
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device4* device = d3d12->getDevice();

    bool ok SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    ok = ok && createRootSignatures();
    ok = ok && createPSOs();

    _ASSERT_EXPR(ok, "Error creating HDRToCubemapPass");
}

HDRToCubemapPass::~HDRToCubemapPass()
{
}

ComPtr<ID3D12Resource> HDRToCubemapPass::generate(D3D12_GPU_DESCRIPTOR_HANDLE hdrSRV, DXGI_FORMAT format, size_t size)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleResources* resources = app->getResources();
    TableDescriptors* table = app->getShaderDescriptors()->getTable();
    ModuleSamplers* samplers = app->getSamplers();

    UINT numMips = UINT(std::log2f(float(size)))+1;

    ComPtr<ID3D12Resource> cubemap = resources->createCubemapRenderTarget(format, size, numMips, Vector4(0.0f, 0.0f, 0.0f, 1.0f), "HDR To Cubemap");

    BEGIN_EVENT(commandList.Get(), "HDR To Cubemap");

    // set necessary state
    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getShaderDescriptors()->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    // set viewport and scissor

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(size), float(size), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(size), LONG(size) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    commandList->SetGraphicsRootDescriptorTable(1, hdrSRV);
    commandList->SetGraphicsRootDescriptorTable(2, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    // create render target view for each face
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(M_HALF_PI, 1.0f, 0.1f, 100.0f);

    for(int i=0; i<6; ++i)
    {
        Matrix viewMatrix = cubemapMesh->getViewMatrix(CubemapMesh::Direction(i));
        Matrix mvpMatrix = (viewMatrix * projMatrix).Transpose();

        commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvpMatrix, 0);

        UINT subResource = D3D12CalcSubresource(0, i, 0, UINT(numMips), 6);

        CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(cubemap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResource);
        commandList->ResourceBarrier(1, &toRT);

        UINT rtvHandle = rtDescriptors->create(cubemap.Get(), i, 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtDescriptors->getCPUHandle(rtvHandle);
        commandList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);

        cubemapMesh->draw(commandList.Get());

        CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(cubemap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResource);
        commandList->ResourceBarrier(1, &toSRV);

        rtDescriptors->release(rtvHandle);
    }


    // Generate mips

    commandList->SetGraphicsRootSignature(mipsRS.Get());
    commandList->SetPipelineState(mipsPSO.Get());

    commandList->SetGraphicsRootDescriptorTable(1, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    for (UINT i = 1; i < numMips; ++i)
    {
        UINT tableDesc = table->alloc();
        UINT dim = (size >> i);

        for (UINT j = 0; j < 6; ++j)
        {
            UINT subResource = D3D12CalcSubresource(i, j, 0, numMips, 6);

            CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(cubemap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, subResource);
            commandList->ResourceBarrier(1, &toRT);

            UINT rtvHandle = rtDescriptors->create(cubemap.Get(), j, i, DXGI_FORMAT_R16G16B16A16_FLOAT);
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtDescriptors->getCPUHandle(rtvHandle);
            commandList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);

            D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(dim), float(dim), 0.0f, 1.0f };
            D3D12_RECT scissor = { 0, 0, LONG(dim), LONG(dim) };
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissor);

            table->createTexture2DSRV(cubemap.Get(), j, (i-1), tableDesc, j);

            commandList->SetGraphicsRootDescriptorTable(0, table->getGPUHandle(tableDesc, j));

            commandList->IASetVertexBuffers(0, 0, nullptr);
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->DrawInstanced(3, 1, 0, 0);

            CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(cubemap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, subResource);
            commandList->ResourceBarrier(1, &toSRV);

            rtDescriptors->release(rtvHandle);
        }

        table->deferRelease(tableDesc);
    }

    END_EVENT(commandList.Get());

    commandList->Close();

    ID3D12CommandList *commandLists[] = {commandList.Get()};
    d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    d3d12->flush();

    commandAllocator->Reset();
    SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    return cubemap;
}

bool HDRToCubemapPass::createRootSignatures()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges[2];
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &tableRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
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


    // Mips RS

    rootParameters[0].InitAsDescriptorTable(1, &tableRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&mipsRS))))
    {
        return false;
    }

    return true;
}

bool HDRToCubemapPass::createPSOs()
{
    auto dataVS = DX::ReadData(L"skyboxVS.cso");
    auto dataPS = DX::ReadData(L"HDRToCubemapPS.cso");

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
    bool ok = SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

    dataVS = DX::ReadData(L"fullscreenVS.cso");
    dataPS = DX::ReadData(L"mipChainPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC mipDesc = {};
    mipDesc.InputLayout = { nullptr, 0};
    mipDesc.pRootSignature = mipsRS.Get();                                                   
    mipDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    mipDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    mipDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    mipDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;                                         
    mipDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    mipDesc.SampleDesc = {1, 0};                                                                    
    mipDesc.SampleMask = 0xffffffff;                                                                
    mipDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               
    mipDesc.DepthStencilState.DepthEnable = FALSE;
    mipDesc.DepthStencilState.DepthEnable = FALSE;
    mipDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         
    mipDesc.NumRenderTargets = 1;                                                                 

    return ok && SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&mipDesc, IID_PPV_ARGS(&mipsPSO)));
}

