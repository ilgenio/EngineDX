#pragma once

struct RenderMesh;

class SkinningPass
{
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:
    SkinningPass();
    ~SkinningPass();

    void record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);

private:
    bool createRootSignature();
    bool createPSO();

    D3D12_GPU_VIRTUAL_ADDRESS buildMatrixPalette(const RenderMesh& mesh);
};