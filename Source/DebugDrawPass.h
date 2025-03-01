#pragma once

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>

class DDRenderInterfaceCoreD3D12;

class DebugDrawPass 
{

public:

    DebugDrawPass(ID3D12Device4* device, ID3D12CommandQueue* uploadQueue, D3D12_CPU_DESCRIPTOR_HANDLE cpuText, D3D12_GPU_DESCRIPTOR_HANDLE gpuText);

    ~DebugDrawPass();

    void record(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height, const Matrix& view ,const Matrix& proj);

private:

    static DDRenderInterfaceCoreD3D12* implementation;
};
