#pragma once

#include "DescriptorHeaps.h"

class ImGuiPass
{
    DescriptorGroup fontGroup;
public:
    ImGuiPass(ID3D12Device2* device, HWND hWnd, D3D12_CPU_DESCRIPTOR_HANDLE cpuFont, D3D12_GPU_DESCRIPTOR_HANDLE gpuFont);
    ~ImGuiPass();

    void startFrame();
    void record(ID3D12GraphicsCommandList* commandList);
};
