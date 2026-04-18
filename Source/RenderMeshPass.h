#pragma once

#include "Material.h"
#include "RenderStructs.h"
#include <span>

struct RenderMesh;

class RenderMeshPass
{
    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_PER_FRAME_CB = 1,
        SLOT_PER_INSTANCE_CB = 2,
        SLOT_DIRECTIONAL_BUFFER = 3,
        SLOT_POINT_BUFFER = 4,
        SLOT_SPOT_BUFFER = 5,
        SLOT_POINT_LIST = 6,
        SLOT_SPOT_LIST = 7,
        SLOT_IBL_TABLE = 8,
        SLOT_TEXTURES_TABLE = 9,
        SLOT_SAMPLERS = 10,
        SLOT_COUNT
    };

    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        Material::Data material;
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:
    RenderMeshPass();
    ~RenderMeshPass();

    bool init(bool useMSAA);
    void render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData);

private:
    bool createRootSignature();
    bool createPSO(bool useMSAA);
};