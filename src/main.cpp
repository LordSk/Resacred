#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include <direct.h> // mkdir
#include <SDL2/SDL.h>

#define GL3W_IMPLEMENTATION
#include "gl3w.h"

#include "rs_base.h"
#include "rs_logger.h"
#include "rs_window.h"
#include "rs_game.h"
#include "rs_file.h"
#include "rs_jobs.h"

#include <common/TracySystem.hpp>

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

#ifdef TRACY_ENABLE
	_mkdir("./captures");
	time_t t = time(0);
	struct tm* tt = localtime(&t);
	String<256> cmdLine;
	cmdLine.setf("@start cmd /c \"..\\profiler\\capture.exe -a localhost -o captures\\capture_%d-%d-%d_%d-%d-%d.tracy\"",
				 tt->tm_mday, tt->tm_mon+1, tt->tm_year+1900,
				 tt->tm_hour, tt->tm_min, tt->tm_sec);

	LOG("Profiler, enabled, capturing...");
	LOG(cmdLine.c_str());
	system(cmdLine.c_str());
#endif

	tracy::SetThreadName(GetCurrentThread(), "Main");

    Window& client = *init_clientWindow();
    client.create(1600, 900);

	const i32 workerCount = MAX(threadGetLogicalProcessorCount() - 3, 1);
	LOG("job system: workerCount=%d", workerCount);
	jobSystemInit(workerCount);

	auto threadGame = threadCreate(thread_game, nullptr);

    const i32 procCount = threadGetLogicalProcessorCount();
	threadSetProcessorAffinity(threadGame, 1);

	while(client.isRunning) {
		client.handleInput();
    }

	client.shutdown();
	threadWaitForCloseTimeout(&threadGame, 1, 1000);
	jobSystemShutdown();

    SDL_Quit();

    return 0;
}
