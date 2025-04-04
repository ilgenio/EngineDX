#pragma once

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

#include <memory>

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

