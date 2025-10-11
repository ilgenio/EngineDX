#pragma once

#include "Material.h"
#include <span>

struct RenderMesh;

class RenderMeshPass
{
    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_PER_FRAME_CB = 1,
        SLOT_PER_INSTANCE_CB = 2,
        SLOT_LIGHTS_TABLE = 3,
        SLOT_IBL_TABLE = 4,
        SLOT_TEXTURES_TABLE = 5,
        SLOT_SAMPLERS = 6,
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
    void render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, D3D12_GPU_VIRTUAL_ADDRESS perFrameData, D3D12_GPU_DESCRIPTOR_HANDLE iblTable, const Matrix& viewProjection);

private:
    bool createRootSignature();
    bool createPSO(bool useMSAA);
};