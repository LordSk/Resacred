#pragma once
#include <gl3w.h>
#include <SDL2/SDL_events.h>
#include "rs_base.h"

void imguiInit(u32 width, u32 height, const char* iniFilename = "imgui.ini");
void imguiUpdate(f64 delta);
void imguiHandleInput(SDL_Event event);
