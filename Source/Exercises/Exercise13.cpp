#include "Globals.h"
#include "Exercise13.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleCamera.h"
#include "ModuleRingBuffer.h"

#include "BasicModel.h"

#include "DebugDrawPass.h"

#include "Skybox.h"
#include "ImGuiPass.h"

#include "ReadData.h"
#include "RenderTexture.h"

Exercise13::Exercise13()
{
}

Exercise13::~Exercise13()
{
}

bool Exercise13::init() 
{
    bool ok = createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadModel();

    if (ok)
    {
        ModuleResources* resources = app->getResources();
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDesc     = descriptors->allocTable();        
        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), false, debugDesc.getCPUHandle(TEX_SLOT_DEBUGDRAW), debugDesc.getGPUHandle(TEX_SLOT_DEBUGDRAW) );
        imguiPass     = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(TEX_SLOT_IMGUI), debugDesc.getGPUHandle(TEX_SLOT_IMGUI));
        skybox        = std::make_unique<Skybox>();
        renderTexture = std::make_unique<RenderTexture>("Exercise13", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f);

        ok = skybox->init("Assets/Textures/footprint_court.hdr", false);

        ModuleCamera* camera = app->getCamera(); 
        camera->setPanning(Vector3(0.0f, 0.5f, 1.7f));
        camera->setPolar(0.0f);
        camera->setAzimuthal(XMConvertToRadians(-10.25f));
    }

    return true;
}

bool Exercise13::cleanUp()
{
    imguiPass.reset();

    return true;
}

void Exercise13::configureDockspace()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockNodeId");
    ImGui::DockSpaceOverViewport(dockspace_id);

    static bool init = true;
    ImVec2 mainSize = ImGui::GetMainViewport()->Size;
    if (init)
    {
        init = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_CentralNode);
        ImGui::DockBuilderSetNodeSize(dockspace_id, mainSize);

        ImGuiID dock_id_left = 0, dock_id_right = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.75f, &dock_id_left, &dock_id_right);
        ImGui::DockBuilderDockWindow("IBL Viewer Options", dock_id_right);
        ImGui::DockBuilderDockWindow("Scene", dock_id_left);

        ImGui::DockBuilderFinish(dockspace_id);
    }
}

void Exercise13::preRender()
{
    imguiPass->startFrame();

    configureDockspace();
    imGuiCommands();
    debugCommands();

    if(canvasSize.x > 0.0f && canvasSize.y > 0.0f)
        renderTexture->resize(unsigned(canvasSize.x), unsigned(canvasSize.y));
}

void Exercise13::debugCommands()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);
}

void Exercise13::imGuiCommands()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    ImGui::Begin("IBL Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ModuleCamera* camera = app->getCamera();
    ImGui::Text("Camera pos: [%.2f, %.2f, %.2f], Camera spherical angles: [%.2f, %.2f]", camera->getPos().x, camera->getPos().y, camera->getPos().z,
        XMConvertToDegrees(camera->getPolar()), XMConvertToDegrees(camera->getAzimuthal()));
    
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);

    ImGui::End();

    bool viewerFocused = false;
    ImGui::Begin("Scene");
    const char* frameName = "Scene Frame";
    ImGuiID id(10);

    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImVec2 min = ImGui::GetWindowContentRegionMin();
    canvasSize = ImVec2(max.x - min.x, max.y - min.y);
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    ImGui::BeginChildFrame(id, canvasSize, ImGuiWindowFlags_NoScrollbar);
    viewerFocused = ImGui::IsWindowFocused();

    if(renderTexture->isValid())
    {
        ImGui::Image((ImTextureID)renderTexture->getSrvHandle().ptr, canvasSize);
    }

    ImGui::EndChildFrame();
    ImGui::End();

    app->getCamera()->setEnable(viewerFocused);
}

void Exercise13::renderModel(ID3D12GraphicsCommandList* commandList)
{
    ModuleCamera* camera = app->getCamera();

    BEGIN_EVENT(commandList, "Model Render Pass");

    PerFrame perFrameData;
    perFrameData.camPos = camera->getPos();
    perFrameData.roughnessLevels = float(skybox->getNumIBLMipLevels());

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    Matrix mvp = model->getModelMatrix() * camera->getView() * ModuleCamera::getPerspectiveProj(float(canvasSize.x) / float(canvasSize.y));
    mvp = mvp.Transpose();

    commandList->SetGraphicsRoot32BitConstants(ROOTPARAM_MVP, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootConstantBufferView(ROOTPARAM_PERFRAME, ringBuffer->allocBuffer(&perFrameData));
    commandList->SetGraphicsRootDescriptorTable(ROOTPARAM_IBL_TABLE, skybox->getIBLTable());
    commandList->SetGraphicsRootDescriptorTable(ROOTPARAM_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    for (const BasicMesh& mesh : model->getMeshes())
    {
        if (UINT(mesh.getMaterialIndex()) < model->getNumMaterials())
        {
            const BasicMaterial& material = model->getMaterials()[mesh.getMaterialIndex()];

            PerInstance perInstance = { model->getModelMatrix().Transpose(), model->getNormalMatrix().Transpose(), material.getMetallicRoughnessMaterial() };

            commandList->SetGraphicsRootConstantBufferView(ROOTPARAM_PERINSTANCE, ringBuffer->allocBuffer(&perInstance));
            commandList->SetGraphicsRootDescriptorTable(ROOTPARAM_MATERIAL_TABLE, material.getTexturesTableDesc().getGPUHandle());

            mesh.draw(commandList);
        }
    }

    END_EVENT(commandList);

}

void Exercise13::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleCamera* camera = app->getCamera();

    BEGIN_EVENT(commandList, "Exercise13 Render to Texture");

    renderTexture->beginRender(commandList);

    float aspect = canvasSize.x / canvasSize.y;

    // skybox render
    skybox->render(commandList, aspect);

    // model render
    renderModel(commandList);

    // debug draw render
    debugDrawPass->record(commandList, uint32_t(canvasSize.x), uint32_t(canvasSize.y), 
                          camera->getView(), ModuleCamera::getPerspectiveProj(aspect));

    renderTexture->endRender(commandList);

    END_EVENT(commandList);

}

void Exercise13::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    if(renderTexture->isValid() && canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        renderToTexture(commandList);
    }

    d3d12->setBackBufferRenderTarget();

    imguiPass->record(commandList);

    d3d12->endFrameRender();
}

bool Exercise13::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[ROOTPARAM_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE iblTableRange, materialTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    iblTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    materialTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 3);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[ROOTPARAM_MVP].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[ROOTPARAM_PERFRAME].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ROOTPARAM_PERINSTANCE].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ROOTPARAM_IBL_TABLE].InitAsDescriptorTable(1, &iblTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ROOTPARAM_MATERIAL_TABLE].InitAsDescriptorTable(1, &materialTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ROOTPARAM_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(ROOTPARAM_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool Exercise13::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

    auto dataVS = DX::ReadData(L"Exercise13VS.cso");
    auto dataPS = DX::ReadData(L"Exercise13PS.cso");

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
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;                                           // our models are counter clock wise
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool Exercise13::loadModel()
{
    model = std::make_unique<BasicModel>();

    model->load("Assets/Models/CompareAmbientOcclusion/CompareAmbientOcclusion.gltf", "Assets/Models/CompareAmbientOcclusion/", BasicMaterial::METALLIC_ROUGHNESS);

    return true;
}

