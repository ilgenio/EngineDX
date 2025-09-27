#pragma once

#include "Material.h"
#include <span>

class RenderMesh;

class RenderMeshPass
{
    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_PER_FRAME_CB = 1,
        SLOT_PER_INSTANCE_CB = 2,
        SLOT_LIGHTS_TABLE = 3,
        SLOT_TEXTURES_TABLE = 4,
        SLOT_SAMPLERS = 5,
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

    bool init();
    void render(std::span<const RenderMesh* const> meshes, D3D12_GPU_VIRTUAL_ADDRESS perFrameData, const Matrix& viewProjection, , const ShaderTableDesc& lightsTableDesc, ID3D12GraphicsCommandList* commandList);

private:
    bool createRootSignature();
    bool createPSO();
};