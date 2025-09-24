#include "Globals.h"

#include "RenderTargetDesc.h"

#include "Application.h"
#include "ModuleRTDescriptors.h"

RenderTargetDesc& RenderTargetDesc::operator=(const RenderTargetDesc& other)
{
    if (this != &other)
    {
        release();
        handle = other.handle;
        refCount = other.refCount;
        addRef();
    }

    return *this;
}

RenderTargetDesc& RenderTargetDesc::operator=(RenderTargetDesc&& other)
{
    if (this != &other)
    {
        release();
        handle = other.handle;
        refCount = other.refCount;
        other.handle = 0;
        other.refCount = nullptr;
    }

    return *this;
}

RenderTargetDesc::operator bool() const
{
    return app->getRTDescriptors()->isValid(handle);
}

void RenderTargetDesc::release()
{
    if (refCount && --(*refCount) == 0)
    {
        app->getRTDescriptors()->release(handle);

        handle = 0;
        refCount = nullptr;
    }
}

void RenderTargetDesc::addRef()
{
    if (refCount) ++(*refCount);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDesc::getCPUHandle() const
{
    return app->getRTDescriptors()->getCPUHandle(handle);
}
