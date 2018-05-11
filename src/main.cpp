#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define GL3W_IMPLEMENTATION
#include "gl3w.h"

#include "rs_base.h"
#include "rs_logger.h"
#include "rs_window.h"
#include "rs_renderer.h"
#include "rs_game.h"
#include "rs_file.h"
#include "rs_resources.h"

i32 main()
{
    LOGGER_INIT();

    LOG("-|--  Resacred  --|-");

    i32 sdl = SDL_Init(SDL_INIT_VIDEO);
    if(sdl != 0) {
        LOG("ERROR: could not init SDL2 (%s)", SDL_GetError());
        return 1;
    }

    Window& client = *init_clientWindow();
    client.create(1600, 900);
    client.dbguiInit();

    auto threadRenderer = threadCreate(thread_renderer, nullptr);
    auto threadGame = threadCreate(thread_game, nullptr);
    auto threadFileIO = threadCreate(thread_fileIO, nullptr);
    const i32 procCount = threadGetLogicalProcessorCount();
    threadSetProcessorAffinity(threadRenderer, procCount-1);
    threadSetProcessorAffinity(threadGame, procCount-2);
    threadSetProcessorAffinity(threadFileIO, procCount-3);

    while(client.clientRunning) {
        client.handleInput();
    }

    client.cleanup();
    resource_deinit();

    threadWaitForClose(&threadGame);
    threadWaitForClose(&threadFileIO);
    threadWaitForClose(&threadRenderer);

    SDL_Quit();

    return 0;
}
