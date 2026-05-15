#pragma once

class BlurPass
{

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> psoHorizontal;
    ComPtr<ID3D12PipelineState> psoVertical;

public:

    BlurPass();
    ~BlurPass();

    void resize(int width, int height);

    void record(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE input, D3D12_GPU_DESCRIPTOR_HANDLE output, 
                int width, int height, int kernelSize, float variance);

private:
    void createRootSignature();
    void createHorizontalPSO();
    void createVerticalPSO();
};