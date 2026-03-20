#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

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

    struct CubicSegment
    {
        Vector3 a;
        Vector3 b;
        Vector3 c;
        Vector3 d;

        Vector3 evaluate(float t) const
        {
            return a * t * t * t + b * t * t + c * t + d;
        }
    };

    std::deque<Segment> segments;
    std::vector<Vertex>  vertices;
    std::vector<SHORT> indices;

    UINT modelIdx = 0;
    UINT trailIdx = UINT_MAX;

    float segmentLifeTime = 0.25f;
    float segmentLength = 0.075f;
    float segmentWidth = 0.35f;
    float maxSegmentAngle = 15.0f;
    UINT numCurveInterpolationPoints = 30;
    bool enableDebugDraw = false;    

    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    ComPtr<ID3D12Resource>       texture;
    ShaderTableDesc              textureDescriptor;  

public:

    bool init() override;
    void update() override;
    void preRender() override;

    void record(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj);

private:
    void createPSO();
    void createRootSignature();
    void centripetalCatmullRom(const Vector3 p[4], CubicSegment& segment, float alpha, float tension) const;
    void generateControlPoints(UINT index, const Matrix& trailWorldTransform, Vector3* topPoints, Vector3* bottomPoints) const;
};
