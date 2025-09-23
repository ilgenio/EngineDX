#include "Globals.h"
#include "Exercise11.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "SingleDescriptors.h"
#include "TableDescriptors.h"       
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleCamera.h"
#include "ModuleRingBuffer.h"

#include "Model.h"

#include "DebugDrawPass.h"

#include "IrradianceMapPass.h"
#include "PrefilterEnvMapPass.h"
#include "EnvironmentBRDFPass.h"
#include "HDRToCubemapPass.h"

#include "ImGuiPass.h"
#include "SkyboxRenderPass.h"

#include "ReadData.h"
#include "RenderTexture.h"

#define CAPTURE_IBL_GENERATION 1

Exercise11::Exercise11()
{
}

Exercise11::~Exercise11()
{
}

bool Exercise11::init() 
{
    bool ok = createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadModel();

    if (ok)
    {
        ModuleResources* resources = app->getResources();
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDrawPass       = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());
        irradianceMapPass   = std::make_unique<IrradianceMapPass>();
        prefilterEnvMapPass = std::make_unique<PrefilterEnvMapPass>();
        environmentBRDFPass = std::make_unique<EnvironmentBRDFPass>();
        hdrToCubemapPass    = std::make_unique<HDRToCubemapPass>();
        skyboxRenderPass    = std::make_unique<SkyboxRenderPass>();

        tableDesc = descriptors->allocTable();        
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), tableDesc.getCPUHandle(0), tableDesc.getGPUHandle(0));
        renderTexture = std::make_unique<RenderTexture>("Exercise11", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f);

        hdrSky = resources->createTextureFromFile(std::wstring(L"Assets/Textures/footprint_court.hdr"));

        if ((ok = hdrSky) == true)
        {
            tableDesc.createTextureSRV(hdrSky.Get(), 1);
        }
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

    Model* model = &models[activeModel];
    
    const Matrix & view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));
    Matrix mvp = model->getModelMatrix() * view * proj;
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

    skyboxRenderPass->record(commandList, tableDesc.getGPUHandle(2), Matrix::CreateFromQuaternion(invRot), proj);

    BEGIN_EVENT(commandList, "Model Render Pass");

    PerFrame perFrameData;
    perFrameData.camPos = camera->getPos();
    perFrameData.roughnessLevels = 8.0f;
    perFrameData.useOnlyIrradiance = useOnlyIrradiance;

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootConstantBufferView(1, ringBuffer->allocBuffer(&perFrameData));
    commandList->SetGraphicsRootDescriptorTable(3, tableDesc.getGPUHandle(3));
    commandList->SetGraphicsRootDescriptorTable(5, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    for (const Mesh& mesh : model->getMeshes())
    {
        if (mesh.getMaterialIndex() < model->getNumMaterials())
        {
            const BasicMaterial& material = model->getMaterials()[mesh.getMaterialIndex()];

            PerInstance perInstance = { model->getModelMatrix().Transpose(), model->getNormalMatrix().Transpose(), material.getMetallicRoughnessMaterial() };

            commandList->SetGraphicsRootConstantBufferView(2, ringBuffer->allocBuffer(&perInstance));
            commandList->SetGraphicsRootDescriptorTable(4, material.getTexturesTableDesc().getGPUHandle());

            mesh.draw(commandList);
        }
    }

    END_EVENT(commandList);

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);

    debugDrawPass->record(commandList, width, height, camera->getView(), proj);

    renderTexture->transitionToSRV(commandList);

    END_EVENT(commandList);

}

void Exercise11::imGuiCommands()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    ImGui::Begin("IBL Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);

    ImGui::Separator();

    ImGui::Checkbox("Use only irradiance", &useOnlyIrradiance);

    ImGui::Separator();

    ImGui::Combo("Model", (int*)&activeModel, "MetallicRoughness\0DamagedHelmet\0");

    ImGui::Separator();

    if (environmentBRDF)
    {
        ImGui::Text("Environment BRDF");
        ImGui::Image((ImTextureID)tableDesc.getCPUHandle(5).ptr, ImVec2(128, 128));
    }

    ImGui::End();

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
        ImGui::Image((ImTextureID)renderTexture->getSRVHandle().ptr, canvasSize);
    }

    ImGui::EndChildFrame();
    ImGui::End();

    app->getCamera()->setEnable(viewerFocused);
}

void Exercise11::render()
{
    imGuiCommands();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleCamera* camera = app->getCamera();

#if CAPTURE_IBL_GENERATION
    
    bool takeCapture = (!irradianceMap || !prefilteredEnvMap || !environmentBRDF || !skybox) && PIXIsAttachedForGpuCapture();
    if (takeCapture)
    {
        PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
    }
#endif 

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    if(!irradianceMap || !prefilterEnvMapPass || !environmentBRDF || !skybox)
    {
        skybox = hdrToCubemapPass->generate(tableDesc.getGPUHandle(1), DXGI_FORMAT_R16G16B16A16_FLOAT, 1024);
        tableDesc.createCubeTextureSRV(skybox.Get(), 2);

        irradianceMap = irradianceMapPass->generate(tableDesc.getGPUHandle(2), 1024);
        tableDesc.createCubeTextureSRV(irradianceMap.Get(), 3);

        prefilteredEnvMap = prefilterEnvMapPass->generate(tableDesc.getGPUHandle(2), 1024, 8);
        tableDesc.createCubeTextureSRV(prefilteredEnvMap.Get(), 4);

        environmentBRDF = environmentBRDFPass->generate(128);
        tableDesc.createTextureSRV(environmentBRDF.Get(), 5);
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

bool Exercise11::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[6] = {};
    CD3DX12_DESCRIPTOR_RANGE iblTableRange, materialTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    iblTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    materialTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 3);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsDescriptorTable(1, &iblTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsDescriptorTable(1, &materialTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(6, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool Exercise11::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                          {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                          {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

    auto dataVS = DX::ReadData(L"Exercise11VS.cso");
    auto dataPS = DX::ReadData(L"Exercise11PS.cso");

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

bool Exercise11::loadModel()
{
    models = std::make_unique<Model[]>(2);

    models[0].load("Assets/Models/MetalRoughSpheres/MetalRoughSpheres.gltf", "Assets/Models/MetalRoughSpheres/", BasicMaterial::METALLIC_ROUGHNESS);
    models[0].setModelMatrix(Matrix::CreateRotationZ(M_HALF_PI) * Matrix::CreateRotationX(-M_HALF_PI) * Matrix::CreateTranslation(Vector3(0.0, 10.0, 0.0)) * Matrix::CreateScale(0.4f));

    models[1].load("Assets/Models/DamagedHelmet/DamagedHelmet.gltf", "Assets/Models/DamagedHelmet/", BasicMaterial::METALLIC_ROUGHNESS);
    models[1].setModelMatrix(Matrix::CreateRotationX(M_HALF_PI) * Matrix::CreateRotationY(M_HALF_PI));

    return true;
}

