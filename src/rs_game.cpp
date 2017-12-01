#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_gpu_resources.h"

#define GPU_TEXTURES_COUNT 500
static GLuint* gpu_textures[GPU_TEXTURES_COUNT];
static TextureDesc2D texDescs[GPU_TEXTURES_COUNT];

i32 thread_game(void* data)
{
    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();


    //pak_tilesRead("../sacred_data/tiles.pak", nullptr);
    DiskTextures diskTextures;
    bool texResult = pak_texturesRead("../sacred_data/texture.pak", &diskTextures);
    assert(texResult);
    defer(MEM_DEALLOC(diskTextures.block));

    GPUres_init(&diskTextures);

    if(!texResult) {
        LOG_ERR("thread_game> pak_texturesRead() failed");
        return 0;
    }

    const i32 VIEW_ID_OFFSET = 2000;

    // upload textures to gpu
    i32 texDiskIds[GPU_TEXTURES_COUNT];
    for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
        texDiskIds[i] = i + VIEW_ID_OFFSET;
    }

    GPUres_requestTextures(texDiskIds, gpu_textures, GPU_TEXTURES_COUNT);

    while(client.running) {
        GPUres_newFrame();

        client.dbguiNewFrameBegin();
#ifdef CONF_DEBUG
        if(client.imguiSetup) {
            GPUres_requestTextures(texDiskIds, gpu_textures, GPU_TEXTURES_COUNT);

            ImGui::Begin("Textures");
            for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
                ImGui::Image((ImTextureID)(intptr_t)*gpu_textures[i],
                             ImVec2(256, 256));
                ImGui::SameLine();
                ImGui::Text("%32s %d", diskTextures.textureName[i + VIEW_ID_OFFSET].data,
                        diskTextures.textureInfo[i + VIEW_ID_OFFSET].type);
            }
            ImGui::End();
            //ImGui::ShowTestWindow();
        }
#endif
        client.dbguiNewFrameEnd();

        CommandList list;
        list.clear();
        list.endFrame();
        renderer_pushCommandList(list);
    }

    // TODO: wait for deinit
    GPUres_deinit();

    LOG("Game> initializated.");
    return 0;
}
