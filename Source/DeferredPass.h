#pragma once

class DeferredPass
{
    enum RootSlots
    {
        SLOT_PER_FRAME_CB = 0,
        SLOT_GBUFFER_TABLE = 1,
        SLOT_LIGHTS_TABLE = 2,
        SLOT_IBL_TABLE = 3,
        SLOT_SAMPLERS = 4,
        SLOT_COUNT
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:
    DeferredPass();
    ~DeferredPass();

    bool init();
    void render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS perFrameData, D3D12_GPU_DESCRIPTOR_HANDLE gbufferTable, D3D12_GPU_DESCRIPTOR_HANDLE lightsTable, D3D12_GPU_DESCRIPTOR_HANDLE iblTable);

private:
    bool createRootSignature();
    bool createPSO();
};