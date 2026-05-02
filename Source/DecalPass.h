#pragma once

#include <span>
#include <memory>

class DecalCubeMesh;
class Decal;
struct RenderData;

class DecalPass 
{
    enum RootSlots
    {
        SLOT_MVP_MATRIX = 0,
        SLOT_DECAL_CONSTANTS = 1,
        SLOT_TEXTURES_TABLE = 2,
        SLOT_SAMPLERS = 3,
        SLOT_COUNT
    };


    struct Constants
    {
        Matrix projection;
        Matrix invView;
        Matrix invModel;
    };


    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
    std::unique_ptr<DecalCubeMesh> decalCubeMesh;

public:

    DecalPass();
    ~DecalPass();

    void render(ID3D12GraphicsCommandList* commandList, std::span<const std::shared_ptr<Decal> > decals, const RenderData& renderData);

private:
    bool createRootSignature();
    bool createPSO();
};