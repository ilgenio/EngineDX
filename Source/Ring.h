#pragma once

class Ring
{
    size_t                 totalMemorySize = 0;
    size_t                 head = 0;
    size_t                 tail = 0;
    size_t                 allocatedInFrame[FRAMES_IN_FLIGHT];
    size_t                 totalAllocated = 0;
    unsigned               currentFrame = 0;
public:
    Ring();
    ~Ring();

    void init(size_t size);
    void updateFrame(unsigned frameIdx);
    size_t alloc(size_t size);
};