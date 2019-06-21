#pragma once
#include "rs_base.h"
#include "rs_thread.h"
#include "rs_array.h"
#include <SDL2/SDL_events.h>

struct Window
{
    struct SDL_Window* window;
	bool isRunning = true;
    i32 width;
    i32 height;

    typedef void (*Proc_InputCallback)(const SDL_Event&, void*);
    struct InputListenner
    {
        Proc_InputCallback callback;
        void* userData;
    };
    Array<InputListenner,16> inputListeners;

    bool create(const i32 width_, const i32 height_);
    void handleInput();
    void swapBuffers();
    void shutdown();

    void addInputCallback(Proc_InputCallback callback, void* userData);

	void dbgUiNewFrame();
};

Window* get_clientWindow();
Window* init_clientWindow();
