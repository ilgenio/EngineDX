#pragma once

class ShaderTableDesc
{
    UINT  handle = 0;
    UINT* refCount = nullptr;

public:

    ShaderTableDesc(UINT handle = 0, UINT* refCount = nullptr) : handle(handle), refCount(refCount) { addRef(); }
    ShaderTableDesc(const ShaderTableDesc& other) : handle(other.handle), refCount(other.refCount) { addRef(); }
    ShaderTableDesc(ShaderTableDesc&& other) noexcept : handle(other.handle), refCount(other.refCount) { other.handle = 0; other.refCount = nullptr; }
    ~ShaderTableDesc() { release(); }

    ShaderTableDesc& operator=(const ShaderTableDesc& other);
    ShaderTableDesc& operator=(ShaderTableDesc&& other);

    operator bool() const;

    void createCBV(ID3D12Resource* resource, UINT8 slot = 0);
    void createTextureSRV(ID3D12Resource* resource, UINT8 slot = 0);
    void createTexture2DSRV(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, UINT8 slot = 0);
    void createTexture2DUAV(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, UINT8 slot = 0);
    void createCubeTextureSRV(ID3D12Resource* resource, UINT8 slot = 0);
    void createNullTexture2DSRV(UINT8 slot = 0);

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT8 slot = 0) const;
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT8 slot = 0) const;

    void reset() { release(); }

private:
    void release();
    void addRef();
};
