#pragma once
#include <gl3w.h>
#include <SDL2/SDL_events.h>
#include "rs_base.h"

struct ImGuiGLSetup;

struct ImGuiGLSetup* imguiInit(u32 width, u32 height, const char* iniFilename = "imgui.ini");
void imguiDeinit(struct ImGuiGLSetup* ims);
void imguiUpdate(struct ImGuiGLSetup* ims, f64 delta);
void imguiHandleInput(struct ImGuiGLSetup* ims, SDL_Event event);
