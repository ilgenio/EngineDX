#pragma once


class ImGuiPass
{
    ComPtr<ID3D12DescriptorHeap> heap;

public:
    ImGuiPass(ID3D12Device2* device, HWND hWnd);
    ~ImGuiPass();

    void startFrame();
    void record(ID3D12GraphicsCommandList* commandList);
};
