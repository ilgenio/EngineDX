#pragma once

#include "Module.h"

#include <deque>

class DemoTrail : public Module
{
    struct Segment
    {
        Matrix transform = Matrix::Identity;
        float lifeTime = 0.0f;
    };

    struct Vertex
    {
        Vector3 position;
        Vector2 texCoord;
        Vector3 colour;
    };

    enum RootParams
    {
        SLOT_MVP,
        SLOT_TEXTURE,
        SLOT_SAMPLERS,
        SLOT_COUNT
    };

    std::deque<Segment> segments;
    std::vector<Vertex>  vertices;
    std::vector<SHORT> indices;

    UINT modelIdx = 0;
    UINT trailIdx = UINT_MAX;

    float segmentLifeTime = 0.25f;
    float segmentLength = 0.2f;
    float segmentWidth = 0.35f;
    bool enableDebugDraw = false;

    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;

public:

    bool init() override;
    void update() override;
    void preRender() override;

    void record(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj);

private:
    void createPSO();
    void createRootSignature();
};
