#include "rs_window.h"
#include "rs_logger.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include <stdio.h>
#include <SDL2/SDL.h>

Window* g_clientWindowPtr = nullptr;

bool Window::create(const i32 width_, const i32 height_)
{
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

#ifdef CONF_ENABLE_UI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL2_Init(window);
#endif

    return true;
}

void Window::handleInput()
{
	SDL_Event event;
	while(SDL_WaitEvent(&event)) {
#ifdef CONF_ENABLE_UI
		ImGui_ImplSDL2_ProcessEvent(&event);
#endif

        if(event.type == SDL_QUIT) {
			isRunning = false;
            return;
        }

        if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.sym == SDLK_ESCAPE) {
				isRunning = false;
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

void Window::dbgUiNewFrame()
{
#ifdef CONF_ENABLE_UI
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
#endif
}

void Window::dbgUiFrameEnd()
{
#ifdef CONF_ENABLE_UI
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

