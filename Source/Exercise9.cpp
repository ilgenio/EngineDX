#include "Globals.h"
#include "Exercise9.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleCamera.h"
#include "DebugDrawPass.h"

#include "CubemapMesh.h"
#include "ReadData.h"

Exercise9::Exercise9()
{

}

Exercise9::~Exercise9()
{
}

bool Exercise9::init() 
{
    cubemapMesh = std::make_unique<CubemapMesh>();

    bool ok = createRootSignature();
    ok = ok && createPSO();

    if (ok)
    {
        ModuleResources* resources = app->getResources();
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());

        cubemap = resources->createTextureFromFile(std::wstring(L"Assets/Textures/cubemap.dds"));

        if ((ok = cubemap) == true)
        {
            cubemapDesc = descriptors->createCubeTextureSRV(cubemap.Get());
        }

        srvTarget = descriptors->createNullTexture2DSRV();
        UINT imguiTextDesc = descriptors->alloc();
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), descriptors->getCPUHandle(imguiTextDesc), descriptors->getGPUHandle(imguiTextDesc));

    }

    return true;
}

bool Exercise9::cleanUp()
{
    imguiPass.reset();

    return true;
}

void Exercise9::preRender()
{
    imguiPass->startFrame();

    resizeRenderTexture();
}

void Exercise9::resizeRenderTexture()
{
    if (canvasSize.x > 0 && canvasSize.y > 0 && (previousSize.x != canvasSize.x || previousSize.x == 0 ||
        previousSize.y != canvasSize.y || previousSize.y == 0))
    {

        if (renderTexture)
        {
            // Ensure previous texture usage is finished
            app->getD3D12()->flush();
        }

        ModuleResources* resources            = app->getResources();
        ModuleShaderDescriptors* descriptors  = app->getShaderDescriptors();
        ModuleRTDescriptors* rtDescriptors    = app->getRTDescriptors();
        ModuleDSDescriptors* dsDescriptors    = app->getDSDescriptors();

        float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
        renderTexture = resources->createRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM, size_t(canvasSize.x), size_t(canvasSize.y), clearColor, "Exercise9 RT");

        // Create RTV.
        rtDescriptors->release(rtvTarget);
        rtvTarget = rtDescriptors->create(renderTexture.Get());

        // Create SRV.
        descriptors->release(srvTarget);
        srvTarget = descriptors->createTextureSRV(renderTexture.Get());

        renderDS = resources->createDepthStencil(DXGI_FORMAT_D32_FLOAT, size_t(canvasSize.x), size_t(canvasSize.y), 1.0f, 0, "Exercise9 DS");

        // Create DSV
        dsDescriptors->release(dsvTarget);
        dsvTarget = dsDescriptors->create(renderDS.Get());

        previousSize = canvasSize;
    }
}

void Exercise9::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    ModuleDSDescriptors* dsDescriptors = app->getDSDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleCamera* camera = app->getCamera();

    unsigned width = unsigned(canvasSize.x);
    unsigned height = unsigned(canvasSize.y);

    const Quaternion& rot = camera->getRot();
    Quaternion invRot;
    rot.Inverse(invRot);
    Matrix view = Matrix::CreateFromQuaternion(invRot);
    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));

    Matrix vp = view * proj;
    vp = vp.Transpose();

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width = float(width);
    viewport.Height = float(height);

    D3D12_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = width;
    scissor.bottom = height;

    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRT);
    
    BEGIN_EVENT(commandList, "Sky Cubemap Render Pass");

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtDescriptors->getCPUHandle(rtvTarget);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsDescriptors->getCPUHandle(dsvTarget);
    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->SetGraphicsRootSignature(rootSignature.Get());

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &vp, 0);
    commandList->SetGraphicsRootDescriptorTable(1, descriptors->getGPUHandle(cubemapDesc));
    commandList->SetGraphicsRootDescriptorTable(2, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);       // set the primitive topology
    commandList->IASetVertexBuffers(0, 1, &cubemapMesh->getVertexBufferView());     // set the vertex buffer (using the vertex buffer view)
    commandList->DrawInstanced(cubemapMesh->getVertexCount(), 1, 0, 0);     // draw the vertex buffer to the render target              

    END_EVENT(commandList);

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    debugDrawPass->record(commandList, width, height, camera->getView(), proj);

    CD3DX12_RESOURCE_BARRIER toPS = CD3DX12_RESOURCE_BARRIER::Transition(renderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toPS);
}

void Exercise9::imGuiCommands()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    bool viewerFocused = false;
    ImGui::Begin("Scene");
    const char* frameName = "Scene Frame";
    ImGuiID id(10);

    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImVec2 min = ImGui::GetWindowContentRegionMin();
    canvasPos = min;
    canvasSize = ImVec2(max.x - min.x, max.y - min.y);
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    ImGui::BeginChildFrame(id, canvasSize, ImGuiWindowFlags_NoScrollbar);
    viewerFocused = ImGui::IsWindowFocused();
    ImGui::Image((ImTextureID)descriptors->getGPUHandle(srvTarget).ptr, canvasSize);
    
    ImGui::EndChildFrame();
    ImGui::End();

    app->getCamera()->setEnable(viewerFocused);

}

void Exercise9::render()
{
    imGuiCommands();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleCamera* camera = app->getCamera();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), pso.Get());

    if(renderTexture)
    {
        renderToTexture(commandList);
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width = float(width);
    viewport.Height = float(height);

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

    imguiPass->record(commandList);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Exercise9::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
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

bool Exercise9::createPSO()
{
    auto dataVS = DX::ReadData(L"skyboxVS.cso");
    auto dataPS = DX::ReadData(L"Exercise9PS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = cubemapMesh->getInputLayoutDesc(); 
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = {1, 0};                                                                    // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;                                           // our models are counter clock wise
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // NOTE: This is important as cubemap Z will be 1 and default comparison is LESS (not equal) 
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}
