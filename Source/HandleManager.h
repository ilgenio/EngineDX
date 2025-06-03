#pragma once

#include <array>

template<size_t size>
requires (size < (1 << 24))
class HandleManager
{
    struct Data
    {
        UINT index     : 24;  // index to allocated/free data
        UINT number    : 8;   // generation number

        Data() : index(size), number(0)  { ; }
        explicit Data(UINT handle) { *reinterpret_cast<UINT*>(this) = handle; }
        operator UINT() { return *reinterpret_cast<UINT*>(this); }
    };

    std::array<Data, size> data;

    UINT firstFree = 0;
    UINT genNumber = 0;

public:

    HandleManager()
    {
        UINT nextIndex = 0;
        for (Data& data : data)
        {
            data.index = ++nextIndex;
            data.number = 0;
        }
    }

    UINT allocHandle()
    {
        _ASSERT_EXPR(firstFree < size, "Out of memory handles");

        if (firstFree < size)
        {
            UINT index = firstFree;

            Data& item = data[index];
            firstFree = item.index;

            genNumber = (genNumber + 1) % (1 << 8);

            // reserve 0 as invalid handle
            if (index == 0 && genNumber == 0)
            {
                ++genNumber;
            }

            item.index = index;
            item.number = genNumber;

            return item;
        }

        return 0;
    }

    void freeHandle(UINT handle)
    {
        _ASSERTE(validHandle(handle));

        UINT index = Data(handle).index;

        Data& item = data[index];    
        item.index = firstFree;
        
        firstFree = index;
    }

    UINT indexFromHandle(UINT handle) const
    {
        _ASSERTE(validHandle(handle));
        return Data(handle).index;
    }

    bool validHandle(UINT handle) const
    {
        Data item(handle);
        return valid(item.index, item.number);
    }

private:

    bool valid(UINT index, UINT genNumber) const 
    { 
        return index < data.size() && data[index].index == index && data[index].number == genNumber;  
    }
};

