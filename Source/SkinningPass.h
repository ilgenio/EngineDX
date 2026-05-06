#pragma once

#include <span>

struct RenderMesh;

class SkinningPass
{
    enum RootParams
    {
        ROOTPARAM_NUM_VERTICES = 0,
        ROOTPARAM_PALETTE,
        ROOTPARAM_PALETTE_NORMAL,
        ROOTPARAM_VERTICES,
        ROOTPARAM_BONE_WEIGHTS,
        ROOTPARAM_MORPH_WEIGHTS,
        ROOTPARAM_MORPH_VERTICES,
        ROOTPARAM_OUTPUT,
        ROOTPARAM_COUNT
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;

    ComPtr<ID3D12Resource> output;

public:

    SkinningPass();
    ~SkinningPass();

    void render(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);
    D3D12_GPU_VIRTUAL_ADDRESS getOutputAddress() const; 

private:
    std::vector<Matrix> copyPalettes(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);
    std::vector<float> copyMorphWeights(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);

    bool buildBuffers();
    bool createRootSignature();
    bool createPSO();
};