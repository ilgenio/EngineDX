#pragma once

#include <span>

struct RenderMesh;

class SkinningPass
{
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;

    ComPtr<ID3D12Resource> upload;
    ComPtr<ID3D12Resource> palettes[FRAMES_IN_FLIGHT];
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