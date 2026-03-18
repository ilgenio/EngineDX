#include "Globals.h"

#include "DemoTrail.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

#include "ReadData.h"

bool DemoTrail::init() 
{
    createRootSignature();
    createPSO();

    ModuleScene* scene = app->getScene();

    // Skybox
    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    // Sword Model
    modelIdx = scene->addModel("Assets/Models/Sword/sword.gltf", "Assets/Models/Sword/");
    auto model = scene->getModel(modelIdx);

    // Trail
    trailIdx = model->findNode("Trail");
    ModuleResources* resources = app->getResources();
    texture = resources->createTextureFromFile(std::wstring(L"Assets/Models/Sword/swoosh.dds"), true);
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    textureDescriptor = descriptors->allocTable();
    textureDescriptor.createTextureSRV(texture.Get());

    // Animation
    UINT animIdx = scene->addClip("Assets/Models/Sword/sword.gltf", 0);
    model->playAnim(scene->getClip(animIdx));

    app->getRender()->addDebugDrawModel(modelIdx);    

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    ModuleRender* render = app->getRender();

    render->addRenderCallback(std::bind(&DemoTrail::record, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    return true;
}

void DemoTrail::update()
{
    // Update delta time

    float deltaTime = float(app->getElapsedMilis()) * 0.001f;

    for (auto it = segments.begin(); it != segments.end(); )
    {
        it->lifeTime -= deltaTime;

        if (it->lifeTime <= 0.0f)
        {
            it = segments.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if(trailIdx == UINT_MAX)
        return;

    // Add new segments

    ModuleScene* scene = app->getScene();
    auto model = scene->getModel(modelIdx);

    const Matrix& trailWorldTransform = model->getWorldTransform(trailIdx);

    if(segments.empty() || Vector3::Distance(segments.back().transform.Translation(), trailWorldTransform.Translation()) > segmentLength)
    {
        Segment newSegment;
        newSegment.transform = trailWorldTransform;
        newSegment.lifeTime = segmentLifeTime;
        segments.push_back(newSegment);
    }
}

void DemoTrail::preRender()
{
    ImGui::Begin("Demo Viewer Options");
    ImGui::SliderFloat("Segment Life Time", &segmentLifeTime, 0.0f, 1.0f);
    ImGui::SliderFloat("Segment Length", &segmentLength, 0.001f, 1.0f);
    ImGui::SliderFloat("Segment Width", &segmentWidth, 0.0f, 1.0f);
    ImGui::Checkbox("Enable debug draw", &enableDebugDraw);
    
    bool timePaused = app->isTimePaused();
    if (ImGui::Checkbox("Pause time", &timePaused))
    {
        app->setTimePaused(timePaused);
    }
    ImGui::End();


    if (enableDebugDraw)
    {
        for (Segment& segment : segments)
        {
            const Matrix& transform = segment.transform;

            Vector3 up = transform.Forward();
            up.Normalize();

            Vector3 down = transform.Backward();
            down.Normalize();

            float overTimeWidth = segment.lifeTime / segmentLifeTime;


            Vector3 top = segment.transform.Translation() + up * segmentWidth * overTimeWidth;
            Vector3 bottom = segment.transform.Translation() + down * segmentWidth * overTimeWidth;

            dd::point(ddConvert(segment.transform.Translation()), dd::colors::White, 5.0f);
            dd::point(ddConvert(top), dd::colors::Green, 5.0f);
            dd::point(ddConvert(bottom), dd::colors::Red, 5.0f);

            dd::line(ddConvert(top), ddConvert(bottom), dd::colors::White);
        }
    }

    // Build vertices
    vertices.clear();
    vertices.reserve(segments.size() * 2);

    for(UINT i = 0, count = UINT(segments.size()); i < count; ++i)
    {
        const Segment& segment = segments[i];
        const Matrix& transform = segment.transform;

        Vector3 up = transform.Forward();
        up.Normalize();

        Vector3 down = transform.Backward();
        down.Normalize();

        float overTimeWidth = segment.lifeTime / segmentLifeTime;

        Vector3 top = segment.transform.Translation() + up * segmentWidth * overTimeWidth;
        Vector3 bottom = segment.transform.Translation() + down * segmentWidth * overTimeWidth;

        float x = float(i) / float(count - 1);

        vertices.push_back({ top, Vector2(x, 0.0f), Vector3(1.0f, 1.0f, 1.0f) });
        vertices.push_back({ bottom, Vector2(x, 1.0f), Vector3(1.0f, 1.0f, 1.0f) });
    }


    // Building trinagle strips 
    indices.clear();
    indices.resize(vertices.size());

    for(UINT i = 0; i < indices.size(); ++i)
    {
        indices[i] = SHORT(i);
    }
}

void DemoTrail::record(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj)
{
    if(vertices.empty() || indices.empty())
        return;

    BEGIN_EVENT(commandList, "Trail Demo Pass");

    Matrix mvp = view * proj;
    mvp = mvp.Transpose();

    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetGraphicsRoot32BitConstants(SLOT_MVP, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

    commandList->SetGraphicsRootDescriptorTable(SLOT_TEXTURE, textureDescriptor.getGPUHandle());

    commandList->SetGraphicsRootDescriptorTable(SLOT_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    // Trail Dynamic Vertex Buffer
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = ringBuffer->alloc(vertices.data(), vertices.size());
    vertexBufferView.SizeInBytes = UINT(sizeof(Vertex) * vertices.size());
    vertexBufferView.StrideInBytes = sizeof(Vertex);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // Trail Dynamic Index Buffer

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = ringBuffer->alloc(indices.data(), indices.size());
    indexBufferView.SizeInBytes = UINT(sizeof(SHORT) * indices.size());
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->DrawIndexedInstanced(UINT(indices.size()), 1, 0, 0, 0);

    END_EVENT(commandList);
}

void DemoTrail::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[SLOT_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE textureTable;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[SLOT_MVP].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[SLOT_TEXTURE].InitAsDescriptorTable(1, &textureTable  , D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(SLOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);
}

void DemoTrail::createPSO()
{
    auto dataVS = DX::ReadData(L"trailVS.cso");
    auto dataPS = DX::ReadData(L"trailPS.cso");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"COLOR",  0, DXGI_FORMAT_R32G32B32_FLOAT,   0, offsetof(Vertex, colour),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = { &inputLayout[0], UINT(std::size(inputLayout)) };



    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;                                                          // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = { 1, 0 };                                                                  // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                                        
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;                         // disable depth writes

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;                                          // enable blending for the first render target
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
}

