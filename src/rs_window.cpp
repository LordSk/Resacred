#include "rs_window.h"
#include "rs_logger.h"
#include "imgui_sdl2_setup.h"
#include "imgui.h"
#include <stdio.h>
#include <SDL2/SDL.h>

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
    while(SDL_PollEvent(&event)) {
        //imguiHandleInput(imguiSetup, event);

        if(event.type == SDL_QUIT) {
            running = false;
        }

        if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
    }
}

void Window::swapBuffers()
{
    SDL_GL_SwapWindow(window);
}

void Window::cleanup()
{
    imguiMutex.lock();
    imguiDeinit(imguiSetup);
    imguiSetup = 0;
    LOG_DBG("Window::cleanup()");
    imguiMutex.unlock();

    SDL_DestroyWindow(window);
}

void Window::dbguiInit()
{
#ifdef CONF_DEBUG
    imguiSetup = imguiInit(width, height, "resacred_imgui.ini");
#endif
}

void Window::dbguiNewFrame()
{
#ifdef CONF_DEBUG
    if(imguiSetup) {
        imguiMutex.lock();
        imguiUpdate(imguiSetup, 0);
        ImGui::ShowTestWindow();
        imguiMutex.unlock();
    }
#endif
}

void Window::dbguiRender()
{
#ifdef CONF_DEBUG
    if(imguiSetup) {
        imguiMutex.lock();
        ImGui::Render();
        imguiMutex.unlock();
    }
#endif
}

void Window::dbguiWaitForInit()
{
#ifdef CONF_DEBUG
    while(!imguiSetup) {
        _mm_pause();
    }
#endif
}

Window* g_clientWindowPtr = nullptr;

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

