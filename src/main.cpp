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

    auto threadRenderer = SDL_CreateThread(thread_renderer, "thread_renderer", nullptr);
    auto threadGame = SDL_CreateThread(thread_game, "thread_game", nullptr);
    auto threadFileIO = SDL_CreateThread(thread_fileIO, "thread_fileIO", nullptr);

    renderer_waitForInit();
    client.dbguiInit();

    while(client.clientRunning) {
        client.handleInput();
    }

    client.cleanup();

    SDL_WaitThread(threadGame, nullptr);
    SDL_WaitThread(threadFileIO, nullptr);

    client.rdrRunning = false;
    SDL_WaitThread(threadRenderer, nullptr);


    resource_deinit();

    SDL_Quit();

    return 0;
}
