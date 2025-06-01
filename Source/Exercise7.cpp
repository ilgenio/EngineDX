#include "Globals.h"
#include "Exercise7.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include "ModuleDescriptors.h"
#include "ModuleRenderTargets.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "Model.h"
#include "Mesh.h"

#include "ReadData.h"

#include "DirectXTex.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include "d3dx12.h"


Exercise7::Exercise7()
{

}

Exercise7::~Exercise7()
{
}

bool Exercise7::init() 
{
    bool ok = createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadModel();

    if(ok)
    {
        ModuleD3D12* d3d12 = app->getD3D12();
        ModuleDescriptors* descriptors = app->getDescriptors();
        ModuleRenderTargets* renderTargets = app->getRenderTargets();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());

        UINT imguiTextDesc = descriptors->allocate();
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), 
            descriptors->getCPUHandle(imguiTextDesc), descriptors->getGPUHandle(imguiTextDesc));

        renderTexture = std::make_unique< DX::RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
        srvTarget = descriptors->allocate();
        rtvTarget = renderTargets->allocate();

        renderTexture->SetClearColor(DirectX::XMVectorSet(0.2f, 0.2f, 0.2f, 1.0f));
       
        renderTexture->SetDevice(d3d12->getDevice(), descriptors->getCPUHandle(srvTarget), renderTargets->getCPUHandle(rtvTarget));
    }
     
    return true;
}

bool Exercise7::cleanUp()
{
    imguiPass.reset();

    return true;
}

void Exercise7::preRender()
{
    imguiPass->startFrame();
    ImGuizmo::BeginFrame();

    ModuleD3D12* d3d12 = app->getD3D12();

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    // Set the viewport size (adjust based on your application)
    ImGuizmo::SetRect(0, 0, float(width), float(height));

    renderTexture->SizeResources(width, height);

}

void Exercise7::imGuiCommands()
{
    ImGui::Begin("Geometry Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::Checkbox("Show guizmo", &showGuizmo);
    ImGui::Text("Model loaded %s with %d meshes and %d materials", model->getSrcFile().c_str(), model->getNumMeshes(), model->getNumMaterials());

    for (const Mesh& mesh : model->getMeshes())
    {
        ImGui::Text("Mesh %s with %d vertices and %d triangles", mesh.getName().c_str(), mesh.getNumVertices(), mesh.getNumIndices() / 3);
    }

    Matrix objectMatrix = model->getModelMatrix();

    ImGui::Separator();
    // Set ImGuizmo operation mode (TRANSLATE, ROTATE, SCALE)
    static ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_T)) gizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) gizmoOperation = ImGuizmo::SCALE;

    ImGui::RadioButton("Translate", (int*)&gizmoOperation, (int)ImGuizmo::TRANSLATE);
    ImGui::SameLine();
    ImGui::RadioButton("Rotate", (int*)&gizmoOperation, ImGuizmo::ROTATE);
    ImGui::SameLine();
    ImGui::RadioButton("Scale", (int*)&gizmoOperation, ImGuizmo::SCALE);

    float translation[3], rotation[3], scale[3];
    ImGuizmo::DecomposeMatrixToComponents((float*)&objectMatrix, translation, rotation, scale);
    bool transform_changed = ImGui::DragFloat3("Tr", translation, 0.1f);
    transform_changed = transform_changed || ImGui::DragFloat3("Rt", rotation, 0.1f);
    transform_changed = transform_changed || ImGui::DragFloat3("Sc", scale, 0.1f);

    if (transform_changed)
    {
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, (float*)&objectMatrix);

        model->setModelMatrix(objectMatrix);
    }

    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Light Direction", reinterpret_cast<float*>(&light.L), 0.1f, -1.0f, 1.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Normalize"))
        {
            light.L.Normalize();
        }
        ImGui::ColorEdit3("Light Colour", reinterpret_cast<float*>(&light.Lc), ImGuiColorEditFlags_NoAlpha);
        ImGui::ColorEdit3("Ambient Colour", reinterpret_cast<float*>(&light.Ac), ImGuiColorEditFlags_NoAlpha);
    }

    for (BasicMaterial& material : model->getMaterials())
    {
        if (material.getMaterialType() == BasicMaterial::PBR_PHONG)
        {
            char tmp[256];
            _snprintf_s(tmp, 255, "Material %s", material.getName());

            if (ImGui::CollapsingHeader(tmp, ImGuiTreeNodeFlags_DefaultOpen))
            {
                PBRPhongMaterialData pbr = material.getPBRPhongMaterial();
                if (ImGui::ColorEdit3("Diffuse Colour", reinterpret_cast<float*>(&pbr.diffuseColour)))
                {
                    material.setPBRPhongMaterial(pbr);
                }

                bool hasTexture = pbr.hasDiffuseTex;
                if (ImGui::Checkbox("Use Texture", &hasTexture))
                {
                    pbr.hasDiffuseTex = hasTexture;
                    material.setPBRPhongMaterial(pbr);
                }
      
                if (ImGui::ColorEdit3("Specular Colour", reinterpret_cast<float*>(&pbr.specularColour)))
                {
                    material.setPBRPhongMaterial(pbr);
                }

                if (ImGui::DragFloat("shininess", &pbr.shininess))
                {
                    material.setPBRPhongMaterial(pbr);
                }
            }
        }
    }

    ImGui::End();

    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleD3D12* d3d12 = app->getD3D12();

    bool viewerFocused = false;
    ImGui::Begin("Scene");
    ImGuiID id(10);

    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImVec2 min = ImGui::GetWindowContentRegionMin();
    ImGui::BeginChildFrame(id, ImVec2(max.x-min.x, max.y-min.y), ImGuiWindowFlags_NoScrollbar);
    viewerFocused = ImGui::IsWindowFocused();
    ImGui::Image((ImTextureID)descriptors->getGPUHandle(srvTarget).ptr, ImVec2(max.x - min.x, max.y - min.y));
    ImGui::EndChildFrame();
    ImGui::End();

    ModuleCamera* camera = app->getCamera();

    if (showGuizmo)
    {
        const Matrix& viewMatrix = camera->getView();
        const Matrix& projMatrix = camera->getProj();

        // Manipulate the object
        ImGuizmo::Manipulate((const float*)&viewMatrix, (const float*)&projMatrix, gizmoOperation, ImGuizmo::LOCAL, (float*)&objectMatrix);
    }

    ImGuiIO& io = ImGui::GetIO();

    camera->setEnable(viewerFocused && !ImGuizmo::IsUsing());

    if (ImGuizmo::IsUsing())
    {
        model->setModelMatrix(objectMatrix);
    }

}

void Exercise7::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleCamera* camera = app->getCamera();
    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleRenderTargets* renderTargets = app->getRenderTargets();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetPipelineState(pso.Get());

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    const Matrix& view = camera->getView();
    const Matrix& proj = camera->getProj();

    Matrix mvp = model->getModelMatrix() * view * proj;
    mvp = mvp.Transpose();

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

    renderTexture->BeginScene(commandList);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = renderTargets->getCPUHandle(rtvTarget);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    PerFrame perFrame;
    perFrame.L = light.L;
    perFrame.Lc = light.Lc;
    perFrame.Ac = light.Ac;
    perFrame.viewPos = camera->getPos();

    perFrame.L.Normalize();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // set the primitive topology
    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
    commandList->SetGraphicsRootConstantBufferView(1, ringBuffer->allocConstantBuffer(&perFrame, alignUp(sizeof(PerFrame), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
    commandList->SetGraphicsRootDescriptorTable(4, samplers->getGPUHanlde(ModuleSamplers::LINEAR_WRAP));

    BEGIN_EVENT(commandList, "Model Render Pass");

    for (const Mesh& mesh : model->getMeshes())
    {
        if (mesh.getMaterialIndex() < model->getNumMaterials())
        {
            commandList->IASetVertexBuffers(0, 1, &mesh.getVertexBufferView());    // set the vertex buffer (using the vertex buffer view)
            const BasicMaterial& material = model->getMaterials()[mesh.getMaterialIndex()];

            UINT tableStartDesc = material.getTexturesTableDescriptor();

            PerInstance perInstance = { model->getModelMatrix(), model->getNormalMatrix(), material.getPBRPhongMaterial() };

            commandList->SetGraphicsRootConstantBufferView(2, ringBuffer->allocConstantBuffer(&perInstance, alignUp(sizeof(PerInstance), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
            commandList->SetGraphicsRootDescriptorTable(3, descriptors->getGPUHandle(tableStartDesc));

            if (mesh.getNumIndices() > 0)
            {
                commandList->IASetIndexBuffer(&mesh.getIndexBufferView());
                commandList->DrawIndexedInstanced(mesh.getNumIndices(), 1, 0, 0, 0);
            }
            else
            {
                commandList->DrawInstanced(mesh.getNumVertices(), 1, 0, 0);
            }
        }
    }

    END_EVENT(commandList);

    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    debugDrawPass->record(commandList, width, height, view, proj);

    renderTexture->EndScene(commandList);
}

void Exercise7::render()
{
    imGuiCommands();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleSamplers* samplers = app->getSamplers();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    renderToTexture(commandList);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();
    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    imguiPass->record(commandList);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if(SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Exercise7::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(5, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool Exercise7::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                              {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}  };

    auto dataVS = DX::ReadData(L"Exercise7VS.cso");
    auto dataPS = DX::ReadData(L"Exercise7PS.cso");

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

bool Exercise7::loadModel()
{
    model = std::make_unique<Model>();
    model->load("Assets/Models/Duck/duck.gltf", "Assets/Models/Duck/", BasicMaterial::PBR_PHONG);
    model->setModelMatrix(Matrix::CreateScale(0.01f, 0.01f, 0.01f));

    return true;
}

