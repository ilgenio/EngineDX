#pragma once

#include "RenderStructs.h"

class DeferredPass
{
    enum RootSlots
    {
        SLOT_PER_FRAME_CB = 0,
        SLOT_SHADOW_VIEW_PROJ = 1,
        SLOT_GBUFFER_TABLE = 2,
        SLOT_DIRECTIONAL_BUFFER = 3,
        SLOT_POINT_BUFFER = 4,
        SLOT_SPOT_BUFFER = 5,
        SLOT_POINT_LIST = 6,
        SLOT_SPOT_LIST = 7, 
        SLOT_IBL_TABLE = 8,
        SLOT_SHADOW_MAP_TABLE = 9,
        SLOT_SAMPLERS = 10,
        SLOT_COUNT
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:
    DeferredPass();
    ~DeferredPass();

    bool init();
    void render(ID3D12GraphicsCommandList* commandList, const RenderData& renderData);

private:
    bool createRootSignature();
    bool createPSO();
};