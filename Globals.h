#pragma once

#define NOMINMAX
#define USING_XINPUT
#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

enum UpdateStatus
{
	UPDATE_CONTINUE = 1,
	UPDATE_STOP,
	UPDATE_ERROR
};
