#pragma once
#include "rs_base.h"
#include "rs_thread.h"

struct Window
{
    struct SDL_Window* window;
    bool running = true;
    i32 width;
    i32 height;

    struct ImGuiGLSetup* imguiSetup;
    MutexSpin imguiMutex;

    bool create(const i32 width_, const i32 height_);
    void handleInput();
    void swapBuffers();
    void cleanup();

    void dbguiInit();
    void dbguiNewFrame();
    void dbguiRender();
    void dbguiWaitForInit();
};

Window* get_clientWindow();
Window* init_clientWindow();
