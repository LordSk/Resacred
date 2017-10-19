#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"

i32 thread_game(void* data)
{
    LOG("Game> initialization...");

    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    while(1) {
        client.dbguiNewFrame();

        CommandList list;
        list.clear();
        list.endFrame();
        renderer_pushCommandList(list);
    }

    LOG("Game> initializated.");
}
