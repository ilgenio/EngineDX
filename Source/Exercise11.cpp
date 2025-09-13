#include "Globals.h"
#include "Exercise11.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleCamera.h"
#include "ModuleRingBuffer.h"

#include "DebugDrawPass.h"

#include "IrradianceMapPass.h"
#include "ImGuiPass.h"
#include "SkyboxRenderPass.h"

#include "CubemapMesh.h"
#include "SphereMesh.h"
#include "ReadData.h"
#include "RenderTexture.h"

#define CAPTURE_IBL_GENERATION 0

Exercise11::Exercise11()
{

}

Exercise11::~Exercise11()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    if (cubemapDesc)
    {
        descriptors->release(cubemapDesc);
    }

    if (imguiTextDesc)
    {
        descriptors->release(imguiTextDesc);
    }

    if (irradianceMapDesc)
    {
        descriptors->release(irradianceMapDesc);
    }
}

bool Exercise11::init() 
{
    sphereMesh = std::make_unique<SphereMesh>(64, 64);

    bool ok = createSphereRS();
    ok = ok && createSpherePSO();

    if (ok)
    {
        ModuleResources* resources = app->getResources();
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());

        irradianceMapPass = std::make_unique<IrradianceMapPass>();
        irradianceMapPass->init();

        skyboxRenderPass = std::make_unique<SkyboxRenderPass>();

        cubemap = resources->createTextureFromFile(std::wstring(L"Assets/Textures/cubemap.dds"));

        if ((ok = cubemap) == true)
        {
            cubemapDesc = descriptors->createCubeTextureSRV(cubemap.Get());
        }

        imguiTextDesc = descriptors->alloc();
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), descriptors->getCPUHandle(imguiTextDesc), descriptors->getGPUHandle(imguiTextDesc));
        renderTexture = std::make_unique<RenderTexture>("Exercise11", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.2f, 0.2f, 0.2f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f);
    }

    return true;
}

bool Exercise11::cleanUp()
{
    imguiPass.reset();

    return true;
}

void Exercise11::preRender()
{
    imguiPass->startFrame();

    renderTexture->resize(unsigned(canvasSize.x), unsigned(canvasSize.y));
}

void Exercise11::renderToTexture(ID3D12GraphicsCommandList* commandList)
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
    
    const Matrix & view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));
    Matrix model = Matrix::CreateScale(1.0f);
    Matrix normalMatrix = model;
    Matrix mvp = model * view * proj;
    mvp = mvp.Transpose();

    BEGIN_EVENT(commandList, "Exercise11 Render to Texture");

    renderTexture->transitionToRTV(commandList);
    renderTexture->bindAsRenderTarget(commandList);
    renderTexture->clear(commandList);

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    skyboxRenderPass->record(commandList, cubemapDesc, Matrix::CreateFromQuaternion(invRot), proj);

    BEGIN_EVENT(commandList, "Exercise11 Render Sphere");

    PerInstance perInstanceData;
    perInstanceData.modelMat = model;
    perInstanceData.normalMat = Matrix::Identity;

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetGraphicsRootSignature(sphereRS.Get());
    commandList->SetPipelineState(spherePSO.Get());
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootConstantBufferView(1, ringBuffer->allocBuffer(&perInstanceData));
    commandList->SetGraphicsRootDescriptorTable(2, descriptors->getGPUHandle(irradianceMapDesc));
    commandList->SetGraphicsRootDescriptorTable(3, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    sphereMesh->draw(commandList);

    END_EVENT(commandList);

    END_EVENT(commandList);

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);

    debugDrawPass->record(commandList, width, height, camera->getView(), proj);

    renderTexture->transitionToSRV(commandList);
}

void Exercise11::imGuiCommands()
{
    ImGui::Begin("IBL Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::End();

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

    if(renderTexture->isValid())
    {
        ImGui::Image((ImTextureID)descriptors->getGPUHandle(renderTexture->getSRVHandle()).ptr, canvasSize);
    }
    
    ImGui::EndChildFrame();
    ImGui::End();

    app->getCamera()->setEnable(viewerFocused);

}

void Exercise11::render()
{
    imGuiCommands();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleCamera* camera = app->getCamera();

#if CAPTURE_IBL_GENERATION
    
    bool takeCapture = !irradianceMap && PIXIsAttachedForGpuCapture();
    if (takeCapture)
    {
        PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
    }
#endif 

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    if(!irradianceMap)
    {
        irradianceMap = irradianceMapPass->generate(cubemapDesc, 512);
        irradianceMapDesc = descriptors->createCubeTextureSRV(irradianceMap.Get());
    }

    if(renderTexture->isValid())
    {
        renderToTexture(commandList);
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
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

#if CAPTURE_IBL_GENERATION
    if (takeCapture)
    {
        PIXEndCapture(TRUE);
    }
#endif 
}

bool Exercise11::createSphereRS()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&sphereRS))))
    {
        return false;
    }

    return true;
}

bool Exercise11::createSpherePSO()
{
    auto dataVS = DX::ReadData(L"Exercise11VS.cso");
    auto dataPS = DX::ReadData(L"Exercise11_irradiancePS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = sphereMesh->getInputLayoutDesc();
    psoDesc.pRootSignature = sphereRS.Get();                                                        // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = {1, 0};                                                                    // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;                                           // our models are counter clock wise
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&spherePSO)));
}
