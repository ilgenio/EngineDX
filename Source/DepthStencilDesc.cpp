#include "Globals.h"

#include "DepthStencilDesc.h"

#include "Application.h"
#include "ModuleDSDescriptors.h"


DepthStencilDesc& DepthStencilDesc::operator=(const DepthStencilDesc& other)
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

DepthStencilDesc& DepthStencilDesc::operator=(DepthStencilDesc&& other)
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

DepthStencilDesc::operator bool() const
{
    return app->getDSDescriptors()->isValid(handle);
}

void DepthStencilDesc::release()
{
    if (refCount && --(*refCount) == 0)
    {
        app->getDSDescriptors()->release(handle);

        handle = 0;
        refCount = nullptr;
    }

}

void DepthStencilDesc::addRef()
{
    if (refCount) ++(*refCount);
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilDesc::getCPUHandle() const
{
    return app->getDSDescriptors()->getCPUHandle(handle);
}
