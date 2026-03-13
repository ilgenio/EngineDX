#pragma once

#include <span>

struct RenderMesh;

class SkinningPass
{
    enum RootParams
    {
        ROOTPARAM_NUM_VERTICES = 0,
        ROOTPARAM_NUM_MORPH_TARGETS,
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

    ComPtr<ID3D12Resource> upload;
    ComPtr<ID3D12Resource> palettesAndWeights[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12Resource> outputs[FRAMES_IN_FLIGHT];

public:

    SkinningPass();
    ~SkinningPass();

    void record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);
    D3D12_GPU_VIRTUAL_ADDRESS getOutputAddress(UINT frameIndex) const { return outputs[frameIndex]->GetGPUVirtualAddress(); }

private:
    UINT copyPalettes(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);

    bool buildBuffers();
    bool createRootSignature();
    bool createPSO();
};