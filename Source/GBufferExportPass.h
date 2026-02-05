#pragma once

#include "GBuffer.h"

#include "Material.h"
#include<span>

struct RenderMesh;

// Renders scene meshes into the G-buffer for deferred shading.
class GBufferExportPass
{
    GBuffer gBuffer;

    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_PER_INSTANCE_CB = 1,
        SLOT_TEXTURES_TABLE = 2,
        SLOT_SAMPLERS = 3,
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

    GBufferExportPass();
    ~GBufferExportPass();

    bool init();
    void resize(UINT sizeX, UINT sizeY);
    void render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, D3D12_GPU_VIRTUAL_ADDRESS skinningBuffer, const Matrix& viewProjection);

    const GBuffer& getGBuffer() const { return gBuffer;  }

private:
    bool createRootSignature();
    bool createPSO();
};