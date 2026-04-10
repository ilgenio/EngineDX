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
    ImGui::Begin("Viewer Options");
    ImGui::SliderFloat("Segment Life Time", &segmentLifeTime, 0.0f, 1.0f);
    ImGui::SliderFloat("Segment Length", &segmentLength, 0.001f, 1.0f);
    ImGui::SliderFloat("Segment Width", &segmentWidth, 0.0f, 1.0f);
    ImGui::SliderAngle("Max Segment Angle", &maxSegmentAngle, 0.0f, 180.0f);
    ImGui::InputInt("Curve Interpolation Points", (int*)&numCurveInterpolationPoints);
    ImGui::Checkbox("Enable debug draw", &enableDebugDraw);
    
    bool timePaused = app->isTimePaused();
    if (ImGui::Checkbox("Pause time", &timePaused))
    {
        app->setTimePaused(timePaused);
    }
    ImGui::End();

    vertices.clear();
    indices.clear();

    if (trailIdx != UINT_MAX)
    {
        ModuleScene* scene = app->getScene();
        auto model = scene->getModel(modelIdx);

        const Matrix& trailWorldTransform = model->getWorldTransform(trailIdx);

        // Building vertices 

        UINT totalVertices = UINT(segments.size() * numCurveInterpolationPoints);
        UINT currentVertex = 0;
        

        for (UINT i = 0, count = UINT(segments.size()); i < count; ++i)
        {
            Vector3 topPoints[4];
            Vector3 bottomPoints[4];
            generateControlPoints(i, trailWorldTransform, &topPoints[0], &bottomPoints[0]);

            // Generate curve
            CubicSegment topCurve, bottomCurve;
            centripetalCatmullRom(topPoints, topCurve, 1.0f, 0.0f);
            centripetalCatmullRom(bottomPoints, bottomCurve, 1.0f, 0.0f);

            for (UINT j = 0; j < numCurveInterpolationPoints; ++j)
            {
                float t = float(j) / float(numCurveInterpolationPoints - 1);

                Vector3 top = topCurve.evaluate(t);
                Vector3 bottom = bottomCurve.evaluate(t);

                float x = float(currentVertex++) / float(totalVertices);

                vertices.push_back({ top, Vector2(x, 0.0f), Vector3(1.0f, 1.0f, 1.0f) });
                vertices.push_back({ bottom, Vector2(x, 1.0f), Vector3(1.0f, 1.0f, 1.0f) });

                if (enableDebugDraw)
                {
                    dd::line(ddConvert(top), ddConvert(bottom), dd::colors::White);
                }
            }
        }

        // Building trinagle strips 
        indices.clear();
        indices.resize(vertices.size());

        for (UINT i = 0; i < indices.size(); ++i)
        {
            indices[i] = SHORT(i);
        }
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

void DemoTrail::centripetalCatmullRom(const Vector3 p[4], CubicSegment& segment, float alpha, float tension) const
{
    const float epsilon = 0.00001f;

    float t0 = 0.0f;
    float t1 = t0 + pow(Vector3::Distance(p[0], p[1]), alpha);
    float t2 = t1 + pow(Vector3::Distance(p[1], p[2]), alpha);
    float t3 = t2 + pow(Vector3::Distance(p[2], p[3]), alpha);

    Vector3 m1 = Vector3::Zero;
    Vector3 m2 = Vector3::Zero;

    if (t0 + epsilon < t1)
    {
        m1 = (1.0f - tension) * (t2 - t1) * ((p[1] - p[0]) / (t1 - t0) - (p[2] - p[0]) / (t2 - t0) + (p[2] - p[1]) / (t2 - t1));
    }

    if (t2 + epsilon < t3)
    {
        m2 = (1.0f - tension) * (t2 - t1) * ((p[2] - p[1]) / (t2 - t1) - (p[3] - p[1]) / (t3 - t1) + (p[3] - p[2]) / (t3 - t2));
    }

    segment.a = 2.0f * (p[1] - p[2]) + m1 + m2;
    segment.b = -3.0f * (p[1] - p[2]) - m1 - m1 - m2;
    segment.c = m1;
    segment.d = p[1];
}

void DemoTrail::generateControlPoints(UINT index, const Matrix& trailWorldTransform, Vector3* topPoints, Vector3* bottomPoints)  const
{
    auto getTopBottom = [](const Matrix& transform, float width, Vector3& top, Vector3& bottom)
        {
            Vector3 up = transform.Forward();
            up.Normalize();
            Vector3 down = transform.Backward();
            down.Normalize();

            top = transform.Translation() + up * width;
            bottom = transform.Translation() + down * width;
        };

    if (index > 0)  // Is the first segment
    {
        getTopBottom(segments[index - 1].transform, segmentWidth * segments[index - 1].lifeTime / segmentLifeTime, topPoints[0], bottomPoints[0]);
    }
    else
    {
        getTopBottom(segments[index].transform, segmentWidth * segments[index].lifeTime / segmentLifeTime, topPoints[0], bottomPoints[0]);
    }

    getTopBottom(segments[index].transform, segmentWidth * segments[index].lifeTime / segmentLifeTime, topPoints[1], bottomPoints[1]);

    if (index + 1 < segments.size()) // Is the last segment
    {
        getTopBottom(segments[index + 1].transform, segmentWidth * segments[index + 1].lifeTime / segmentLifeTime, topPoints[2], bottomPoints[2]);
    }
    else
    {
        getTopBottom(trailWorldTransform, segmentWidth, topPoints[2], bottomPoints[2]);
    }

    if (index + 2 < segments.size()) // Is the last or the penultimate segment
    {
        getTopBottom(segments[index + 2].transform, segmentWidth * segments[index + 2].lifeTime / segmentLifeTime, topPoints[3], bottomPoints[3]);
    }
    else
    {
        getTopBottom(trailWorldTransform, segmentWidth, topPoints[3], bottomPoints[3]);
    }
};

