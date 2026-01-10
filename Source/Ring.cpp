#include "Globals.h"
#include "Ring.h"


Ring::Ring()
{

}

Ring::~Ring()
{

}

void Ring::init(size_t size)
{
    totalMemorySize = size;
    head = 0;
    tail = 0;
    currentFrame = 0;
    totalAllocated = 0;

    for(int i=0; i< FRAMES_IN_FLIGHT; ++i)
    {
        allocatedInFrame[i] = 0;
    }
}

void Ring::updateFrame(unsigned frameIdx)
{
    currentFrame = frameIdx;

    tail = (tail+allocatedInFrame[currentFrame])%totalMemorySize;
    totalAllocated -= allocatedInFrame[currentFrame];

    allocatedInFrame[currentFrame] = 0;
}

size_t Ring::alloc(size_t size)
{
    _ASSERT_EXPR(size < (totalMemorySize-totalAllocated), L"Out of memory, please allocate more memory at initialisation");

    if(tail < head)
    {
        size_t availableToEnd = size_t(totalMemorySize-head);
        if(availableToEnd >= size)
        {
            size_t address = head;
            head += size;

            allocatedInFrame[currentFrame] += size;
            totalAllocated += size;

            return address; 

        }
        else
        {
            // Force contiguous memory, allocate to end and try again with tail >= head
            head = 0;
            allocatedInFrame[currentFrame] += availableToEnd;
            totalAllocated += availableToEnd;
        }
    }

    _ASSERTE(head <= tail);
    _ASSERT_EXPR(size < (totalMemorySize - totalAllocated), L"Out of memory, please allocate more memory at initialisation");

    size_t address = head;
    head += size;

    allocatedInFrame[currentFrame] += size;
    totalAllocated += size;

    return address; 
}
