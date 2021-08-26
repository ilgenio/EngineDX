#pragma once

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>

template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

class Engine
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

    Engine();
    ~Engine();

    bool init(HWND wnd);
    void update();
    void render();

    void resize();
    void toogleFullscreen();
    void flush();

    const TimeInfo& getTimeInfo() const { return timeInfo; }

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

    void getWindowSize(unsigned& width, unsigned& height);
    void clear(float clearColor[4]);
    void present();
};
