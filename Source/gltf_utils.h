#pragma once

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"


inline bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t count, const tinygltf::Model& model, int index)
{
    const tinygltf::Accessor& accessor = model.accessors[index];
    size_t defaultStride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

    if (count == accessor.count && defaultStride == elemSize)
    {
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        const uint8_t* bufferData = reinterpret_cast<const uint8_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        size_t bufferStride = view.byteStride == 0 ? defaultStride : view.byteStride;

        for (uint32_t i = 0; i < count; ++i)
        {
            memcpy(data, bufferData, elemSize);
            data += stride;
            bufferData += bufferStride;
        }

        return true;
    }

    return false;
}

inline bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t count, const tinygltf::Model& model, const std::map<std::string, int>& attributes, const char* accesorName)
{
    const auto& it = attributes.find(accesorName);
    if (it != attributes.end())
    {
        return loadAccessorData(data, elemSize, stride, count, model, it->second);
    }

    return false;
}

template <class T>
inline void loadAccessorTyped(std::unique_ptr<T[]> &data, uint &count, const tinygltf::Model &model, int index)
{
    const tinygltf::Accessor &accessor = model.accessors[index];

    count = uint(accessor.count);
    data = std::make_unique<T[]>(count);

    return loadAccessorData(reinterpret_cast<uint8_t*>(data.get()), sizeof(T), sizeof(T), count, model, index);
}

