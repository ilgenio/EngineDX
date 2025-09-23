#pragma once

#include "HandleManager.h"

template<size_t size>
class DeferredFreeHandleManager : public HandleManager<size>
{
    struct DeferredFree
    {
        UINT frame = 0;
        UINT handle = 0;
    };

    std::array<DeferredFree, size> deferredFrees;
    UINT deferredFreeCount = 0;

public:

    void deferRelease(UINT handle, UINT currentFrame)
    {
        _ASSERT_EXPR(HandleManager<size>::validHandle(handle), L"Invalid handle");

        for (DeferredFree& df : deferredFrees)
        {
            if (df.handle == handle)
            {
                df.frame = currentFrame;
                return;
            }
        }

        deferredFrees[deferredFreeCount++] = { currentFrame, handle};
    }

    void collectGarbage(UINT completedFrame)
    {
        for (size_t i = 0; i < deferredFreeCount; )
        {
            DeferredFree& df = deferredFrees[i];
            if (completedFrame >= df.frame)
            {
                HandleManager<size>::freeHandle(df.handle);
                deferredFreeCount--;
                deferredFrees[i] = deferredFrees[deferredFreeCount];
            }
            else
            {
                ++i;
            }
        }
    }

    void forceCollectGarbage()
    {
        for (size_t i = 0; i < deferredFreeCount; ++i)
        {
            DeferredFree& df = deferredFrees[i];
            HandleManager<size>::freeHandle(df.handle);
        }

        deferredFreeCount = 0;
    }
};
