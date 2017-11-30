#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"

#define GPU_TEXTURES_COUNT 500
static GLuint gpu_textures[GPU_TEXTURES_COUNT];
static TextureDesc2D texDescs[GPU_TEXTURES_COUNT];


/*
TODO:
- Investigate texture type 4

*/







i32 thread_game(void* data)
{
    LOG("Game> initialization...");

    //pak_tilesRead("../sacred_data/tiles.pak", nullptr);
    PakTexture* pakTextures;
    assert(pak_texturesRead("../sacred_data/texture.pak", &pakTextures));

    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    // upload textures to gpu
    CommandList cmds;
    for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
        i32 id = i + 0;
        texDescs[i].width = pakTextures[id].width;
        texDescs[i].height = pakTextures[id].height;
        texDescs[i].internalFormat = GL_RGBA8;
        texDescs[i].dataFormat = GL_RGBA;
        texDescs[i].data =  pakTextures[id].data;
        cmds.createTexture2D(&texDescs[i], &gpu_textures[i]);
    }
    cmds.execute();
    renderer_pushCommandList(cmds);

    while(1) {
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            ImGui::Begin("Textures");
            for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
                if(pakTextures[i].typeId == 6) {
                    ImGui::Image((ImTextureID)(intptr_t)gpu_textures[i],
                                 ImVec2(texDescs[i].width, texDescs[i].height));
                    ImGui::SameLine();
                    ImGui::Text("%32s %d", pakTextures[i].filename, pakTextures[i].typeId);
                }
            }
            ImGui::End();
            //ImGui::ShowTestWindow();
        }
        client.dbguiNewFrameEnd();

        CommandList list;
        list.clear();
        list.endFrame();
        renderer_pushCommandList(list);
    }

    LOG("Game> initializated.");
}
