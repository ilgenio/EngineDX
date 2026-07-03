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

public:

    SkinningPass();
    ~SkinningPass();

    void render(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes);

private:

    void copyPalette(const RenderMesh& mesh, std::vector<Matrix>& palettes);
    void copyMorphWeights(const RenderMesh& mesh, std::vector<float>& morphWeights);

    bool createRootSignature();
    bool createPSO();
};