#pragma once

#define NOMINMAX
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

#define USING_XINPUT
#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

enum UpdateStatus
{
	UPDATE_CONTINUE = 1,
	UPDATE_STOP,
	UPDATE_ERROR
};

#define LOG(format, ...) log(__FILE__, __LINE__, format, __VA_ARGS__);
void log(const char file[], int line, const char* format, ...);

#define FRAMES_IN_FLIGHT 3

#include "debug_draw.hpp"
inline const ddVec3& ddConvert(const Vector3& v) { return reinterpret_cast<const ddVec3&>(v); }
inline const ddMat4x4& ddConvert(const Matrix& m) { return reinterpret_cast<const ddMat4x4&>(m); }

