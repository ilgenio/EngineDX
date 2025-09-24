#pragma once

class RenderTargetDesc
{
    UINT  handle = 0;
    UINT* refCount = nullptr;

public:
    RenderTargetDesc() = default;
    RenderTargetDesc(UINT handle, UINT* refCount) : handle(handle), refCount(refCount) { addRef(); }
    RenderTargetDesc(const RenderTargetDesc& other) : handle(other.handle), refCount(other.refCount) { addRef(); }
    RenderTargetDesc(RenderTargetDesc&& other) noexcept : handle(other.handle), refCount(other.refCount) { other.handle = 0; other.refCount = nullptr; }
    ~RenderTargetDesc() { release(); }

    RenderTargetDesc& operator=(const RenderTargetDesc& other);
    RenderTargetDesc& operator=(RenderTargetDesc&& other);

    operator bool() const;
    void reset() { release(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle() const;

private:
    void release();
    void addRef();
};