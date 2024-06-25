#pragma once

#include "DescriptorHeaps.h"

class ImGuiPass
{
    DescriptorGroup fontGroup;
public:
    ImGuiPass(ID3D12Device2* device, HWND hWnd);
    ~ImGuiPass();

    void startFrame();
    void record(ID3D12GraphicsCommandList* commandList);
};
