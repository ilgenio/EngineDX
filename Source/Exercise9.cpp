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

#include "SkyboxRenderPass.h"
#include "ReadData.h"
#include "RenderTexture.h"

Exercise9::Exercise9()
{

}

Exercise9::~Exercise9()
{
    if(cubemapDesc)
    {
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        descriptors->release(cubemapDesc);
    }

    if (imguiTextDesc)
    {
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        descriptors->release(imguiTextDesc);
    }
}

bool Exercise9::init() 
{
    ModuleResources* resources = app->getResources();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleD3D12* d3d12 = app->getD3D12();

    debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());
    skyboxRenderPass = std::make_unique<SkyboxRenderPass>();

    cubemap = resources->createTextureFromFile(std::wstring(L"Assets/Textures/cubemap.dds"));

    if (cubemap)
    {
        cubemapDesc = descriptors->createCubeTextureSRV(cubemap.Get());
    }

    renderTexture = std::make_unique<RenderTexture>("Exercise9", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.2f, 0.2f, 0.2f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f);

    imguiTextDesc = descriptors->alloc();
    imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), descriptors->getCPUHandle(imguiTextDesc), descriptors->getGPUHandle(imguiTextDesc));

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

    renderTexture->resize(unsigned(canvasSize.x), unsigned(canvasSize.y));
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

    BEGIN_EVENT(commandList, "Sky Cubemap Render Pass");

    renderTexture->transitionToRTV(commandList);
    renderTexture->bindAsRenderTarget(commandList);
    renderTexture->clear(commandList);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    const Quaternion& rot = camera->getRot();
    Quaternion invRot;
    rot.Inverse(invRot);
    Matrix view = Matrix::CreateFromQuaternion(invRot);
    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));

    skyboxRenderPass->record(commandList, cubemapDesc, view, proj);

    END_EVENT(commandList);

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    debugDrawPass->record(commandList, width, height, camera->getView(), proj);

    renderTexture->transitionToSRV(commandList);
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

    if (renderTexture->isValid())
    {
        ImGui::Image((ImTextureID)descriptors->getGPUHandle(renderTexture->getSRVHandle()).ptr, canvasSize);
    }

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
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    if(renderTexture->isValid())
    {
        renderToTexture(commandList);
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
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
}
