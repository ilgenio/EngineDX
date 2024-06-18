#pragma once

#include "Module.h"

#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>

class ModuleD3D12 : public Module
{   

private:

    HWND                                hWnd = NULL ;
    ComPtr<IDXGIFactory5>               factory;
    ComPtr<IDXGIAdapter4>               adapter;
    ComPtr<ID3D12Device5>               device;

    ComPtr<IDXGISwapChain4>             swapChain;
    ComPtr<ID3D12DescriptorHeap>        rtDescriptorHeap;
    ComPtr<ID3D12Resource>              backBuffers[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12DescriptorHeap>        dsDescriptorHeap;
    ComPtr<ID3D12Resource>              depthStencilBuffer;

    ComPtr<ID3D12CommandAllocator>      commandAllocators[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12GraphicsCommandList4>  commandList;
    ComPtr<ID3D12CommandQueue>          drawCommandQueue;

    ComPtr<ID3D12Fence1>                drawFence;
    HANDLE                              drawEvent = NULL;
    unsigned                            drawFenceCounter = 0;
    unsigned                            drawFenceValues[FRAMES_IN_FLIGHT] = { 0, 0, 0 };

    bool                                allowTearing = false;
    bool                                supportsRT = false;
    unsigned                            currentBackBufferIdx = 0;

    unsigned                            windowWidth  = 0;
    unsigned                            windowHeight = 0;
    bool                                fullscreen   = false;
    RECT                                lastWindowRect;
    

public:

    ModuleD3D12(HWND hWnd);
    ~ModuleD3D12();

    bool                        init() override;
    bool                        cleanUp() override;

    UpdateStatus                preUpdate() override;
    UpdateStatus                update() override;
    UpdateStatus                postUpdate() override;

    void                        resize();
    void                        toogleFullscreen();
    void                        flush();

    IDXGISwapChain4*            getSwapChain()        { return swapChain.Get();  }
    ID3D12Device2*              getDevice()           { return device.Get(); }
    ID3D12GraphicsCommandList*  getCommandList()      { return commandList.Get(); }
    ID3D12CommandAllocator*     getCommandAllocator() { return commandAllocators[currentBackBufferIdx].Get(); }
    ID3D12Resource*             getBackBuffer()       { return backBuffers[currentBackBufferIdx].Get(); }
    ID3D12CommandQueue*         getDrawCommandQueue() { return drawCommandQueue.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor();

    void                        signalDrawQueue();

    unsigned                    getWindowWidth() const { return windowWidth; }
    unsigned                    getWindowHeight() const { return windowHeight; }

private:
    void getWindowSize(unsigned& width, unsigned& height);
private:

    void enableDebugLayer();
    bool createFactory();
    bool createDevice(bool useWarp);
    bool setupInfoQueue();
    bool createDrawCommandQueue();
    bool createSwapChain();
    bool createRenderTargets();
    bool createDepthStencil();
    bool createCommandList();
    bool createDrawFence();
};
