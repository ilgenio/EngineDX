#pragma once

#include <span>

class RenderMesh;

class RenderMeshPass
{
    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_PER_FRAME_CB = 1,
        SLOT_PER_INSTANCE_CB = 2,
        SLOT_IBL_TABLE = 3,
        SLOT_MATERIAL_TABLE = 4,
        SLOT_SAMPLERS = 5,
        SLOT_COUNT
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:
    RenderMeshPass();
    ~RenderMeshPass();

    bool init();
    void render(std::span<const RenderMesh*> meshes);

private:
    bool createRootSignature();
    bool createPSO();
};