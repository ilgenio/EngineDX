#include "Globals.h"
#include "Exercise8.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "ModuleResources.h"
#include "Model.h"
#include "Mesh.h"

#include "ReadData.h"
#include "Math.h"

#include "DirectXTex.h"
#include <d3d12.h>
#include "d3dx12.h"

Exercise8::Exercise8()
{

}

Exercise8::~Exercise8()
{
}

bool Exercise8::init() 
{
    bool ok = createRootSignature();
    ok = ok && createPSO();
    ok = ok && loadModel();

    if(ok)
    {
        ModuleD3D12* d3d12 = app->getD3D12();
        ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue());

        UINT imguiTextDesc = descriptors->alloc();
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), 
            descriptors->getCPUHandle(imguiTextDesc), descriptors->getGPUHandle(imguiTextDesc));

        srvTarget = descriptors->createNullTexture2DSRV();

        ZeroMemory(&pointLight, sizeof(Point));
        ZeroMemory(&spotLight, sizeof(Spot));

        ambient.Lc = Vector3::One * (0.1f);

        dirLight.Ld = Vector3::One * (-0.5f);
        dirLight.Lc = Vector3::One;
        dirLight.intenisty = 1.0f;

        pointLight.Lp = Vector3(1.5f, 2.5f, 0.0f);
        pointLight.sqRadius = 16.0f;
        pointLight.intensity = 2.0f;
        pointLight.Lc = Vector3::One;

        spotLight.Lp = Vector3(0.0f, 2.5f, 1.5f);
        spotLight.Ld = Vector3(-0.06f, -0.62f, -0.79f);
        spotLight.inner = 0.93f;
        spotLight.outer = 0.8f;
        spotLight.sqRadius = 25.0f;
        spotLight.intensity = 2.0f;
        spotLight.Lc = Vector3::One;
    }
     
    return true;
}

bool Exercise8::cleanUp()
{
    imguiPass.reset();

    return true;
}

void Exercise8::preRender()
{
    imguiPass->startFrame();
    ImGuizmo::BeginFrame();

    resizeRenderTexture();

}

void Exercise8::resizeRenderTexture()
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
        renderTexture = resources->createRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM, size_t(canvasSize.x), 
            size_t(canvasSize.y), clearColor, "Exercise8 RT");

        renderDS = resources->createDepthStencil(DXGI_FORMAT_D32_FLOAT, size_t(canvasSize.x), 
            size_t(canvasSize.y), 1.0f, 0, "Exercise8 DS");

        // Create RTV.
        rtDescriptors->release(rtvTarget);
        rtvTarget = rtDescriptors->create(renderTexture.Get());

        // Create SRV.
        descriptors->release(srvTarget);
        srvTarget = descriptors->createTextureSRV(renderTexture.Get());

        // Create DSV
        dsDescriptors->release(dsvTarget);
        dsvTarget = dsDescriptors->create(renderDS.Get());

        previousSize = canvasSize;
    }
}

void Exercise8::imGuiDirection(Vector3& dir)
{
    float azimuth;
    float elevation;
    euclideanToSpherical(dir, azimuth, elevation);

    while (azimuth < 0.0f) azimuth += TWO_PI;
    while (elevation < 0.0f) elevation += TWO_PI;

    ImGui::Text("Direction (%g, %g, %g)", dir.x, dir.y, dir.z);
    bool change = ImGui::SliderAngle("Dir azimuth", &azimuth, 0.0f, 360.0f);
    change = ImGui::SliderAngle("Dir elevation", &elevation, 1.0f, 179.0f) || change;

    if (change)
    {
        sphericalToEuclidean(azimuth, elevation, dir);
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Normalize"))
    {
        dir.Normalize();
    }
}

void Exercise8::imGuiDirectional(Directional &dirLight)
{
    imGuiDirection(dirLight.Ld);
    ImGui::ColorEdit3("Colour", reinterpret_cast<float *>(&dirLight.Lc));
    ImGui::DragFloat("Intensity", &dirLight.intenisty, 0.1f, 0.0f, 1000.0f);

    if (ddLight)
    {
        ImGui::DragFloat("DD arrow distance", &ddDistance,  0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("DD arrow szie", &ddSize, 0.1f, 0.0f, 10.0f);
    }
}

void Exercise8::imGuiPoint(Point &pointLight)
{
    ImGui::DragFloat3("Position", reinterpret_cast<float *>(&pointLight.Lp), 0.1f);
    ImGui::ColorEdit3("Colour", reinterpret_cast<float *>(&pointLight.Lc));
    ImGui::DragFloat("Intensity", &pointLight.intensity, 0.1f, 0.0f, 1000.0f);
    float radius = sqrtf(pointLight.sqRadius);
    if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
    {
        pointLight.sqRadius = radius * radius;
    }
}

void Exercise8::imGuiSpot(Spot &spotLight)
{
    ImGui::DragFloat3("Position", reinterpret_cast<float *>(&spotLight.Lp), 0.1f);
    imGuiDirection(spotLight.Ld);

    ImGui::ColorEdit3("Colour", reinterpret_cast<float *>(&spotLight.Lc));
    ImGui::DragFloat("Intensity", &spotLight.intensity, 0.1f, 0.0f, 1000.0f);

    float innerConeAngle = acosf(spotLight.inner);
    float outerConeAngle = acosf(spotLight.outer);
    if (ImGui::SliderAngle("Inner Cone Angle", &innerConeAngle, 0.0f, 90.0f))
    {
        spotLight.inner = cosf(innerConeAngle);
        spotLight.outer = std::min(spotLight.outer, spotLight.inner);
    }
    if (ImGui::SliderAngle("Outer Cone Angle", &outerConeAngle, 0.0f, 90.0f))
    {
        spotLight.outer = cosf(outerConeAngle);
        spotLight.inner = std::max(spotLight.inner, spotLight.outer);
    }

    ImGui::DragFloat("Radius", &spotLight.sqRadius, 0.1f, 0.1f, 1000.0f);
}

void Exercise8::ddDirectional(Directional &dirLight, float distance, float size)
{
    dd::arrow(ddConvert(-dirLight.Ld * (distance + size)), ddConvert(-dirLight.Ld*distance), ddConvert(dirLight.Lc*dirLight.intenisty), 0.1f);
}

void Exercise8::ddPoint(Point &pointLight)
{
    dd::sphere(ddConvert(pointLight.Lp), ddConvert(pointLight.Lc * pointLight.intensity), sqrtf(pointLight.sqRadius));
}

void Exercise8::ddSpot(Spot &spotLight)
{
    dd::arrow(ddConvert(spotLight.Lp), ddConvert(spotLight.Lp + spotLight.Ld* sqrtf(spotLight.sqRadius)), ddConvert(spotLight.Lc * spotLight.intensity), 0.1f);

    dd::cone(ddConvert(spotLight.Lp), ddConvert(spotLight.Ld * sqrtf(spotLight.sqRadius)), ddConvert(spotLight.Lc * spotLight.intensity), sqrtf(spotLight.sqRadius)*tanf(acosf(spotLight.outer)), 0.0f);
}

void Exercise8::imGuiCommands()
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

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("Ambient colour", (float*)&ambient.Lc);
        ImGui::Combo("Light Type", (int*)&lightType, "Directional\0Point\0Spot");
        ImGui::Checkbox("Draw Debug", &ddLight);

        switch (lightType)
        {
        case LIGHT_DIRECTIONAL:
            imGuiDirectional(dirLight);
            if(ddLight) ddDirectional(dirLight, ddDistance, ddSize);
            break;
        case LIGHT_POINT:
        {
            imGuiPoint(pointLight);
            if(ddLight) ddPoint(pointLight);
            break;
        }
        case LIGHT_SPOT:
            imGuiSpot(spotLight);
            if(ddLight) ddSpot(spotLight);
            break;
        }
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

    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleD3D12* d3d12 = app->getD3D12();

    ModuleCamera* camera = app->getCamera();

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
    
    if (showGuizmo)
    {
        const Matrix& viewMatrix = camera->getView();
        Matrix projMatrix = ModuleCamera::getPerspectiveProj(float(canvasSize.x) / float(canvasSize.y));

        // Manipulate the object
        ImGuizmo::SetRect(cursorPos.x, cursorPos.y, canvasSize.x, canvasSize.y);
        ImGuizmo::SetDrawlist();
        ImGuizmo::Manipulate((const float*)&viewMatrix, (const float*)&projMatrix, gizmoOperation, ImGuizmo::LOCAL, (float*)&objectMatrix);
    }

    ImGui::EndChildFrame();
    ImGui::End();

    ImGuiIO& io = ImGui::GetIO();

    camera->setEnable(viewerFocused && !ImGuizmo::IsUsing());

    if (ImGuizmo::IsUsing())
    {
        model->setModelMatrix(objectMatrix);
    }
}

void Exercise8::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleCamera* camera = app->getCamera();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    ModuleDSDescriptors* dsDescriptors = app->getDSDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetPipelineState(pso.Get());

    unsigned width = unsigned(canvasSize.x);
    unsigned height = unsigned(canvasSize.y);

    const Matrix& view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));

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

    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(renderTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRT);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtDescriptors->getCPUHandle(rtvTarget);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsDescriptors->getCPUHandle(dsvTarget);

    PerFrame perFrame;
    perFrame.ambient  = ambient;
    perFrame.numDirLights = lightType == LIGHT_DIRECTIONAL ? 1 : 0;
    perFrame.numPointLights = lightType == LIGHT_POINT ? 1 : 0;
    perFrame.numSpotLights = lightType == LIGHT_SPOT ? 1 : 0;
    perFrame.viewPos = camera->getPos();

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
    commandList->SetGraphicsRootConstantBufferView(1, ringBuffer->allocBuffer(&perFrame, alignUp(sizeof(PerFrame), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
    commandList->SetGraphicsRootShaderResourceView(3, ringBuffer->allocBuffer(&dirLight, alignUp(sizeof(Directional), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
    commandList->SetGraphicsRootShaderResourceView(4, ringBuffer->allocBuffer(&pointLight, alignUp(sizeof(Point), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
    commandList->SetGraphicsRootShaderResourceView(5, ringBuffer->allocBuffer(&spotLight, alignUp(sizeof(Spot), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));

    commandList->SetGraphicsRootDescriptorTable(7, samplers->getGPUHanlde(ModuleSamplers::LINEAR_WRAP));

    BEGIN_EVENT(commandList, "Model Render Pass");

    for (const Mesh& mesh : model->getMeshes())
    {
        if (mesh.getMaterialIndex() < model->getNumMaterials())
        {
            commandList->IASetVertexBuffers(0, 1, &mesh.getVertexBufferView());    // set the vertex buffer (using the vertex buffer view)
            const BasicMaterial& material = model->getMaterials()[mesh.getMaterialIndex()];

            UINT tableStartDesc = material.getTexturesTableDescriptor();

            PerInstance perInstance = { model->getModelMatrix(), model->getNormalMatrix(), material.getPBRPhongMaterial() };

            commandList->SetGraphicsRootConstantBufferView(2, ringBuffer->allocBuffer(&perInstance, alignUp(sizeof(PerInstance), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)));
            commandList->SetGraphicsRootDescriptorTable(6, descriptors->getGPUHandle(tableStartDesc));

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

    CD3DX12_RESOURCE_BARRIER toPS = CD3DX12_RESOURCE_BARRIER::Transition(renderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toPS);
}

void Exercise8::render()
{
    imGuiCommands();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    if (renderTexture)
    {
        renderToTexture(commandList);
    }

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

bool Exercise8::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[8] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsShaderResourceView(0);
    rootParameters[4].InitAsShaderResourceView(1);
    rootParameters[5].InitAsShaderResourceView(2);
    rootParameters[6].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[7].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(8, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool Exercise8::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                              {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}  };

    auto dataVS = DX::ReadData(L"Exercise8VS.cso");
    auto dataPS = DX::ReadData(L"Exercise8PS.cso");

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

bool Exercise8::loadModel()
{
    model = std::make_unique<Model>();
    model->load("Assets/Models/Duck/duck.gltf", "Assets/Models/Duck/", BasicMaterial::PBR_PHONG);
    model->setModelMatrix(Matrix::CreateScale(0.01f, 0.01f, 0.01f));

    return true;
}

