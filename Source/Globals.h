#pragma once

#define USING_XINPUT
#define NOMINMAX
#define INITGUID

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

#include "SimpleMath.h"

#include <assert.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

#define LOG(format, ...) log(__FILE__, __LINE__, format, __VA_ARGS__);
void log(const char file[], int line, const char* format, ...);

#define FRAMES_IN_FLIGHT 3

#include "debug_draw.hpp"
inline const ddVec3& ddConvert(const Vector3& v) { return reinterpret_cast<const ddVec3&>(v); }
inline const ddMat4x4& ddConvert(const Matrix& m) { return reinterpret_cast<const ddMat4x4&>(m); }

inline size_t alignUp(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}
