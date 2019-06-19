#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define GL3W_IMPLEMENTATION
#include "gl3w.h"

#include "rs_base.h"
#include "rs_logger.h"
#include "rs_window.h"
#include "rs_game.h"

/*
 * MAIN THREAD
 *
 * Inits client window
 * Starts all the other threads
 *
 * Handles user input (keyboard, mouse, gamepad, etc..)
 *
 */

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

	auto threadGame = threadCreate(thread_game, nullptr);
	/*auto threadFileIO = threadCreate(thread_fileIO, nullptr);
    const i32 procCount = threadGetLogicalProcessorCount();
    threadSetProcessorAffinity(threadGame, procCount-2);
	threadSetProcessorAffinity(threadFileIO, procCount-3);*/

	while(client.isRunning) {
		client.handleInput();
    }

	client.cleanup();
	/*resource_deinit();

    threadWaitForClose(&threadGame);
	threadWaitForClose(&threadFileIO);*/

    SDL_Quit();

    return 0;
}
