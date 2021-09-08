#pragma once

#include "Module.h"

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>

template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

class ModuleRender : public Module
{   
public:
    struct TimeInfo
    {
        uint64_t frameCount  = 0;
        std::chrono::milliseconds elapsed;
    };

private:
    typedef  std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

    enum { SWAP_BUFFER_COUNT = 3 };

    HWND                                hWnd = NULL ;
    ComPtr<IDXGIFactory5>               factory;
    ComPtr<IDXGIAdapter4>               adapter;
    ComPtr<ID3D12Device2>               device;

    ComPtr<IDXGISwapChain4>             swapChain;
    ComPtr<ID3D12DescriptorHeap>        rtDescriptorHeap;
    ComPtr<ID3D12Resource>              backBuffers[SWAP_BUFFER_COUNT];

    ComPtr<ID3D12CommandAllocator>      commandAllocators[SWAP_BUFFER_COUNT];
    ComPtr<ID3D12GraphicsCommandList>   commandList;
    ComPtr<ID3D12CommandQueue>          drawCommandQueue;

    ComPtr<ID3D12Fence1>                drawFence;
    HANDLE                              drawEvent = NULL;
    unsigned                            drawFenceCounter = 0;
    unsigned                            drawFenceValues[SWAP_BUFFER_COUNT] = { 0, 0, 0 };

    bool                                allowTearing = false;
    TimeInfo                            timeInfo;
    TimePoint                           lastTime;
    unsigned                            currentBackBufferIdx = 0;

    unsigned                            windowWidth  = 0;
    unsigned                            windowHeight = 0;
    bool                                fullscreen   = false;
    RECT                                lastWindowRect;
    

public:

    ModuleRender(HWND hWnd);
    ~ModuleRender();

    bool                        init();
    UpdateStatus                preUpdate();
    UpdateStatus                update();
    UpdateStatus                postUpdate();

    void                        resize();
    void                        toogleFullscreen();
    void                        flush();

    const TimeInfo&             getTimeInfo() const   { return timeInfo; }

    ID3D12Device2*              getDevice()           { return device.Get(); }
    ID3D12GraphicsCommandList*  getCommandList()      { return commandList.Get(); }
    ID3D12CommandAllocator*     getCommandAllocator() { return commandAllocators[currentBackBufferIdx].Get(); }
    ID3D12CommandQueue*         getDrawCommandQueue() { return drawCommandQueue.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE getRenderTarget();

    void                        executeCommandList();
    void                        signalDrawQueue();

    void                        getWindowSize(unsigned& width, unsigned& height);
private:

    void enableDebugLayer();
    bool createFactory();
    bool createDevice(bool useWarp);
    bool setupInfoQueue();
    bool createDrawCommandQueue();
    bool createSwapChain();
    bool createRenderTargets();
    bool createCommandList();
    bool createDrawFence();
};
