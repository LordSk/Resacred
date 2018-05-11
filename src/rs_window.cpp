#include "rs_window.h"
#include "rs_logger.h"
#include "imgui_sdl2_setup.h"
#include "imgui.h"
#include <stdio.h>
#include <SDL2/SDL.h>

Window* g_clientWindowPtr = nullptr;

bool Window::create(const i32 width_, const i32 height_)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    window = SDL_CreateWindow("Resacred",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width_, height_,
                              SDL_WINDOW_OPENGL);

    if(!window) {
        LOG("ERROR: can't create SDL2 window (%s)",  SDL_GetError());
        return false;
    }

    width = width_;
    height = height_;

    return true;
}

void Window::handleInput()
{
    SDL_Event event;
    while(SDL_WaitEvent(&event)) {
#ifdef CONF_ENABLE_UI
        imguiHandleInput(event);
#endif

        if(event.type == SDL_QUIT) {
            clientRunning = false;
            return;
        }

        if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.sym == SDLK_ESCAPE) {
                clientRunning = false;
                return;
            }
        }

        for(auto l : inputListeners) {
            l.callback(event, l.userData);
        }
    }
}

void Window::swapBuffers()
{
    SDL_GL_SwapWindow(window);
}

void Window::cleanup()
{
    LOG_DBG("Window::cleanup()");
    SDL_DestroyWindow(window);
    g_clientWindowPtr = nullptr;
}

void Window::addInputCallback(Window::Proc_InputCallback callback, void* userData)
{
    InputListenner listenner;
    listenner.callback = callback;
    listenner.userData = userData;
    inputListeners.pushPOD(&listenner);
}

void Window::dbguiInit()
{
#ifdef CONF_ENABLE_UI
    imguiInit(width, height, "resacred_imgui.ini");
#endif
}

void Window::dbguiNewFrame()
{
#ifdef CONF_ENABLE_UI
    imguiUpdate(0);
#endif
}

void Window::dbguiFrameEnd()
{
#ifdef CONF_ENABLE_UI
    ImGui::EndFrame();
    ImGui::Render();
#endif
}


Window* get_clientWindow()
{
    return g_clientWindowPtr;
}

Window* init_clientWindow()
{
    assert(g_clientWindowPtr == nullptr);
    static Window window;
    g_clientWindowPtr = &window;
    return g_clientWindowPtr;
}

