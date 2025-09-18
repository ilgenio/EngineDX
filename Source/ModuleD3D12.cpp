#include "Globals.h"
#include "ModuleD3D12.h"

#include "d3dx12.h"

ModuleD3D12::ModuleD3D12(HWND wnd) : hWnd(wnd)
{

}

ModuleD3D12::~ModuleD3D12()
{
    flush();
}

bool ModuleD3D12::init()
{   
#if defined(_DEBUG)
    enableDebugLayer();
#endif 

    getWindowSize(windowWidth, windowHeight);

    bool ok = createFactory(); 
    ok = ok && createDevice(false);
    
#if defined(_DEBUG)
    ok = ok && setupInfoQueue();
#endif 
    ok = ok && createDrawCommandQueue();

    ok = ok && createSwapChain();
    ok = ok && createRenderTargets();
    ok = ok && createDepthStencil();
    ok = ok && createCommandList();
    ok = ok && createDrawFence();

    if (ok)
    {
        currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
    }

    return ok;

}

bool ModuleD3D12::cleanUp()
{
    if(drawEvent) CloseHandle(drawEvent);
    drawEvent = NULL;

    return true;
}

void ModuleD3D12::preRender()
{
    currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
    if(drawFenceValues[currentBackBufferIdx] != 0)
    {
        drawFence->SetEventOnCompletion(drawFenceValues[currentBackBufferIdx], drawEvent);
        WaitForSingleObject(drawEvent, INFINITE);

        drawFenceValues[currentBackBufferIdx];
        lastCompletedFrame = std::max(frameValues[currentBackBufferIdx], lastCompletedFrame);
    }

    frameIndex++;
    frameValues[currentBackBufferIdx] = frameIndex;
    
    commandAllocators[currentBackBufferIdx]->Reset();
}

void ModuleD3D12::postRender()

{
    swapChain->Present(0, allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

    signalDrawQueue();
}

UINT ModuleD3D12::signalDrawQueue()
{
    drawFenceValues[currentBackBufferIdx] = ++drawFenceCounter;
    drawCommandQueue->Signal(drawFence.Get(), drawFenceValues[currentBackBufferIdx]);

    return drawFenceCounter;
}

void ModuleD3D12::resize()
{
    unsigned width, height;
    getWindowSize(width, height);

    if (width != windowWidth || height != windowHeight)
    {
        windowWidth  = width;
        windowHeight = height;

        flush();

        for (unsigned i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            backBuffers[i].Reset();
            drawFenceValues[i] = 0;
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        
        bool ok = SUCCEEDED(swapChain->GetDesc(&swapChainDesc));
        ok = ok && SUCCEEDED(swapChain->ResizeBuffers(FRAMES_IN_FLIGHT, windowWidth, windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        if (windowWidth > 0 && windowHeight > 0)
        {
            ok = ok && createRenderTargets();
            ok = ok && createDepthStencil();
        }
    }
}

void ModuleD3D12::toogleFullscreen()
{
    fullscreen = !fullscreen;

    if(fullscreen)
    {

        GetWindowRect(hWnd, &lastWindowRect);

        UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        SetWindowLongW(hWnd, GWL_STYLE, windowStyle);

        HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(hMonitor, &monitorInfo);

        SetWindowPos(hWnd, HWND_TOP,
                     monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.top,
                     monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(hWnd, SW_MAXIMIZE);
    }
    else
    {
        SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        SetWindowPos(hWnd, HWND_NOTOPMOST,
                     lastWindowRect.left,
                     lastWindowRect.top,
                     lastWindowRect.right - lastWindowRect.left,
                     lastWindowRect.bottom - lastWindowRect.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(hWnd, SW_NORMAL);
    }
}

void ModuleD3D12::flush()
{
    drawCommandQueue->Signal(drawFence.Get(), ++drawFenceCounter);

    drawFence->SetEventOnCompletion(drawFenceCounter, drawEvent);
    WaitForSingleObject(drawEvent, INFINITE);    
}

void ModuleD3D12::enableDebugLayer()
{
    ComPtr<ID3D12Debug> debugInterface;

    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));

    debugInterface->EnableDebugLayer();
}

bool ModuleD3D12::createFactory()
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    return SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

bool ModuleD3D12::createDevice(bool useWarp)
{
    bool ok = true;

    if (useWarp)
    {
        ComPtr<IDXGIAdapter1> adapter;
        ok = ok && SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        ok = ok && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    }
    else
    {
        ComPtr<IDXGIAdapter4> adapter;
        SIZE_T maxDedicatedVideoMemory = 0;
        factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
        ok = SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));        
    }

    if(ok)
    {
        // Check for tearing to disable V-Sync needed for some monitors
        BOOL tearing = FALSE;
        factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing));

        allowTearing = tearing == TRUE;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
        HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            supportsRT = true;
        }
    }

    return ok;
}

bool ModuleD3D12::setupInfoQueue()
{
    ComPtr<ID3D12InfoQueue> pInfoQueue;

    bool ok = SUCCEEDED(device.As(&pInfoQueue));
    if (ok)
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};
 
        // Suppress messages based on their severity level
        //D3D12_MESSAGE_SEVERITY Severities[] =
        //{
            //D3D12_MESSAGE_SEVERITY_INFO
        //};
 
        // Suppress individual messages by their ID
        //D3D12_MESSAGE_ID DenyIds[] = {
            //D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            //D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            //D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        //};
 
        //D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        //NewFilter.DenyList.NumSeverities = _countof(Severities);
        //NewFilter.DenyList.pSeverityList = Severities;
        //NewFilter.DenyList.NumIDs = _countof(DenyIds);
        //NewFilter.DenyList.pIDList = DenyIds;
 
        //ok = SUCCEEDED(pInfoQueue->PushStorageFilter(&NewFilter));
    }

    return ok;
}

bool ModuleD3D12::createDrawCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
 
    return SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&drawCommandQueue)));
}

bool ModuleD3D12::createSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1>     swapChain1;
    bool ok = SUCCEEDED(factory->CreateSwapChainForHwnd(drawCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

    ok = ok && SUCCEEDED(swapChain1.As(&swapChain));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ok = SUCCEEDED(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    return ok;
}

bool ModuleD3D12::createDepthStencil()
{
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, windowWidth, windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer)));

    if (ok)
    {
        depthStencilBuffer->SetName(L"Depth/Stencil Texture");

        // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ok = SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap)));

        if (ok) dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");
    }

    if (ok)
    {
        device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    return ok;
}

bool ModuleD3D12::createRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = FRAMES_IN_FLIGHT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
 
    bool ok = SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtDescriptorHeap)));

    if(ok)
    {
        UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; ok && i < FRAMES_IN_FLIGHT; ++i)
        {
            ok = SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));

            if (ok)
            {
                backBuffers[i]->SetName(L"BackBuffer");

                device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
            }

            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    return ok;
}

bool ModuleD3D12::createCommandList()
{
    bool ok = true;
    
    for(unsigned i = 0; ok && i < FRAMES_IN_FLIGHT; ++i)
    {
        ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&commandAllocators[i])));
    }

    ok = ok && SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Close());

    return ok;
}

bool ModuleD3D12::createDrawFence()
{
    bool ok = SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&drawFence)));

    if (ok)
    {
        drawEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        ok = drawEvent != NULL;
    }

    return ok;
}

void ModuleD3D12::getWindowSize(unsigned &width, unsigned &height)
{
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);

    width  = unsigned(clientRect.right - clientRect.left);
    height = unsigned(clientRect.bottom - clientRect.top);    
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getRenderTargetDescriptor()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIdx, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getDepthStencilDescriptor()
{
    return dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

