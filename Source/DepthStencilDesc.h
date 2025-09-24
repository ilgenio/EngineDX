#pragma once 

class DepthStencilDesc
{
    UINT  handle = 0;
    UINT* refCount = nullptr;

public:
    DepthStencilDesc() = default;
    DepthStencilDesc(UINT handle, UINT* refCount) : handle(handle), refCount(refCount) { addRef(); }
    DepthStencilDesc(const DepthStencilDesc& other) : handle(other.handle), refCount(other.refCount) { addRef(); }
    DepthStencilDesc(DepthStencilDesc&& other) noexcept : handle(other.handle), refCount(other.refCount) { other.handle = 0; other.refCount = nullptr; }
    ~DepthStencilDesc() { release(); }

    DepthStencilDesc& operator=(const DepthStencilDesc& other);
    DepthStencilDesc& operator=(DepthStencilDesc&& other);

    operator bool() const;
    void reset() { release(); }

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle() const;

private:
    void release();
    void addRef();

};