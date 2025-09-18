#pragma once


class EnvironmentBRDFPass 
{
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12RootSignature>       rootSignature;
    ComPtr<ID3D12PipelineState>       pso;

public:
    EnvironmentBRDFPass();
    ~EnvironmentBRDFPass();

    ComPtr<ID3D12Resource> generate(size_t size);

private:
    bool createRootSignature();
    bool createPSO();
};