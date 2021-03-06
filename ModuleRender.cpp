#include "ModuleRender.h"

#include "d3dx12.h"

ModuleRender::ModuleRender(HWND wnd) : hWnd(wnd)
{

}

typedef HRESULT(WINAPI* LPDXGIGETDEBUGINTERFACE)(REFIID, void**);

ModuleRender::~ModuleRender()
{
    flush();
}

bool ModuleRender::init()
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
    ok = ok && createCommandList();
    ok = ok && createDrawFence();

    if (ok)
    {
        lastTime = std::chrono::high_resolution_clock::now();
        currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
    }

    return ok;

}

UpdateStatus ModuleRender::preUpdate()
{
    TimePoint currentTime = std::chrono::high_resolution_clock::now();
    timeInfo.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
    ++timeInfo.frameCount;
    lastTime = currentTime;

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::update()
{
    // TODO: Make it working without update
    currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
    if(drawFenceValues[currentBackBufferIdx] != 0)
    {
        drawFence->SetEventOnCompletion(drawFenceValues[currentBackBufferIdx], drawEvent);
        WaitForSingleObject(drawEvent, INFINITE);
    }

    commandAllocators[currentBackBufferIdx]->Reset();
    commandList->Reset(commandAllocators[currentBackBufferIdx].Get(), nullptr);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    if(SUCCEEDED(commandList->Close()))
    {
        executeCommandList();
    }

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::postUpdate()
{
    commandList->Reset(commandAllocators[currentBackBufferIdx].Get(), nullptr);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    if(SUCCEEDED(commandList->Close()))
    {
        executeCommandList();
    }
    
    swapChain->Present(0, allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

    signalDrawQueue();

    return UPDATE_CONTINUE;
}

void ModuleRender::signalDrawQueue()
{
    drawFenceValues[currentBackBufferIdx] = ++drawFenceCounter;
    drawCommandQueue->Signal(drawFence.Get(), drawFenceValues[currentBackBufferIdx]);
}

void ModuleRender::resize()
{
    unsigned width, height;
    getWindowSize(width, height);

    if (width != windowWidth || height != windowHeight)
    {
        windowWidth  = width;
        windowHeight = height;

        flush();

        for (unsigned i = 0; i < SWAP_BUFFER_COUNT; ++i)
        {
            backBuffers[i].Reset();
            drawFenceValues[i] = 0;
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

        bool ok = SUCCEEDED(swapChain->GetDesc(&swapChainDesc));
        ok = ok && SUCCEEDED(swapChain->ResizeBuffers(SWAP_BUFFER_COUNT, windowWidth, windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
        ok = ok && createRenderTargets();
    }
}

void ModuleRender::toogleFullscreen()
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

void ModuleRender::flush()
{
    
    drawCommandQueue->Signal(drawFence.Get(), ++drawFenceCounter);

    drawFence->SetEventOnCompletion(drawFenceCounter, drawEvent);
    WaitForSingleObject(drawEvent, INFINITE);    
}

void ModuleRender::enableDebugLayer()
{
    ComPtr<ID3D12Debug> debugInterface;

    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));

    debugInterface->EnableDebugLayer();
}

bool ModuleRender::createFactory()
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    return SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

bool ModuleRender::createDevice(bool useWarp)
{
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    ComPtr<ID3D12Device2> d3d12Device2;

    bool ok = true;

    if (useWarp)
    {
        ok = ok && SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ok = ok && SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4));
        ok = ok && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12Device2)));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; ok && factory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            ComPtr<ID3D12Device2> tmpDevice;

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory
            // is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&tmpDevice))))
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ok = SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4));
                d3d12Device2 = tmpDevice;
            }
        }
    }

    if(ok)
    {
        // Check for tearing to disable V-Sync needed for some monitors
        BOOL tearing = FALSE;
        factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing));

        allowTearing = tearing == TRUE;
        adapter = dxgiAdapter4;
        device = d3d12Device2;
    }

    return ok;
}

bool ModuleRender::setupInfoQueue()
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

bool ModuleRender::createDrawCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type =     D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
 
    return SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&drawCommandQueue)));
}

bool ModuleRender::createSwapChain()
{
    // TODO: handle resize

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = windowWidth;
    swapChainDesc.Height = windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_BUFFER_COUNT;
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

bool ModuleRender::createRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = SWAP_BUFFER_COUNT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
 
    bool ok = SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtDescriptorHeap)));

    if(ok)
    {
        UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; ok && i < SWAP_BUFFER_COUNT; ++i)
        {
            ok = SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));

            if (ok)
            {
                device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
            }

            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    return ok;
}

bool ModuleRender::createCommandList()
{
    bool ok = true;
    
    for(unsigned i = 0; ok && i < SWAP_BUFFER_COUNT; ++i)
    {
        ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&commandAllocators[i])));
    }

    ok = ok && SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Close());

    return ok;
}

bool ModuleRender::createDrawFence()
{
    bool ok = SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&drawFence)));

    if (ok)
    {
        drawEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        ok = drawEvent != NULL;
    }

    return ok;
}

D3D12_CPU_DESCRIPTOR_HANDLE ModuleRender::getRenderTarget()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIdx, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

void ModuleRender::executeCommandList()
{
    ID3D12CommandList *const gfxCommandList = commandList.Get();
    drawCommandQueue->ExecuteCommandLists(1, &gfxCommandList);
}

void ModuleRender::getWindowSize(unsigned &width, unsigned &height)
{
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);

    width  = unsigned(clientRect.right - clientRect.left);
    height = unsigned(clientRect.bottom - clientRect.top);    
}
