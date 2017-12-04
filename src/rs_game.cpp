#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_gpu_resources.h"


// TODO:
// Investigate textures not loading (GPUres)





#define GPU_TEXTURES_COUNT 500
static GLuint* gpu_textures[GPU_TEXTURES_COUNT];
static TextureDesc2D texDescs[GPU_TEXTURES_COUNT];

void ui_videoInfo()
{
    static i32 dedicated, availMemory, currentAvailMem, evictionCount, evictedMem;
    CommandList list;
    list.queryVramInfo(&dedicated, &availMemory, &currentAvailMem, &evictionCount, &evictedMem);
    renderer_pushCommandList(list);

    ImGui::Begin("Video info");

    ImGui::Text("Total available: %dkb", availMemory);
    ImGui::Text("Current Avilable: %dkb", currentAvailMem);
    ImGui::ProgressBar(1.0 - (currentAvailMem / (f64)availMemory));

    ImGui::End();
}

i32 thread_game(void* data)
{
    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    memset(gpu_textures, 0, sizeof(gpu_textures));

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

    i32 viewIdOffset = 0;
    i32 texDiskIds[GPU_TEXTURES_COUNT];

    while(client.running) {
        GPUres_newFrame();

        client.dbguiNewFrameBegin();
#ifdef CONF_DEBUG
        if(client.imguiSetup) {
            ImGui::Begin("Textures");
            ImGui::SliderInt("offset", &viewIdOffset, 0, 2500);

            ImGui::BeginChild("texture_list");

            for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
                texDiskIds[i] = i + viewIdOffset + 1;
            }
            GPUres_requestTextures(texDiskIds, gpu_textures, GPU_TEXTURES_COUNT);


            for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
                ImGui::Image((ImTextureID)(intptr_t)*gpu_textures[i],
                             ImVec2(256, 256));
                if((i + 1)%4 != 0) {
                    ImGui::SameLine();
                }
            }
            ImGui::EndChild();

            ImGui::End();

            ui_videoInfo();

            ImGui::ShowTestWindow();
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
