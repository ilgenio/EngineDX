#pragma once

#include "Module.h"

#include <memory>

namespace DirectX { class Keyboard; class Mouse; class GamePad;  }

class ModuleInput : public Module
{
public:

    ModuleInput();

private:
    std::unique_ptr<Keyboard> keyboard;
    std::unique_ptr<Mouse> mouse;
    std::unique_ptr<GamePad> gamePad;
};
