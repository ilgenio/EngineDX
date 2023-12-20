#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"

#include "Keyboard.h"
#include "Mouse.h"

ModuleInput::ModuleInput() 
{
    keyboard = std::make_unique<Keyboard>();
    mouse = std::make_unique<Mouse>();
}


