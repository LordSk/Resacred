#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_gpu_resources.h"


#define PAGE_TEXTURES_COUNT 500
static GLuint* gpu_textures[PAGE_TEXTURES_COUNT];
static TextureDesc2D texDescs[PAGE_TEXTURES_COUNT];
static DiskTextures diskTextures;
static i32 texDiskIds[PAGE_TEXTURES_COUNT];
static i32 pageId = 0;
static i32 dedicated = 0, availMemory = 0, currentAvailMem = 0, evictionCount = 0, evictedMem = 0;

void ui_videoInfo()
{
    ImGui::Begin("Video info");

    ImGui::Text("Total available: %dkb", availMemory);
    ImGui::Text("Current Avilable: %dkb", currentAvailMem);
    ImGui::ProgressBar(1.0 - (currentAvailMem / (f64)availMemory));

    ImGui::End();
}

void ui_textureBrowser()
{
    ImGui::Begin("Textures");
    ImGui::SliderInt("page", &pageId, 0, 3000/PAGE_TEXTURES_COUNT - 1);

    ImGui::BeginChild("texture_list");

    for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
        ImGui::BeginGroup();
        ImGui::Image((ImTextureID)(intptr_t)*gpu_textures[i],
                     ImVec2(256, 256));
        i32 diskTexId = texDiskIds[i];
        ImGui::Text("%s (%d/%d)", diskTextures.textureName[diskTexId],
                    diskTextures.textureInfo[diskTexId].width,
                    diskTextures.textureInfo[diskTexId].height);
        ImGui::EndGroup();

        if((i + 1)%4 != 0) {
            ImGui::SameLine();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

i32 thread_game(void* data)
{
    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    memset(gpu_textures, 0, sizeof(gpu_textures));

    bin_WorldRead("../sacred_data/World.bin");
    pak_tilesRead("../sacred_data/tiles.pak", nullptr);
    bool texResult = pak_texturesRead("../sacred_data/texture.pak", &diskTextures);
    assert(texResult);
    defer(MEM_DEALLOC(diskTextures.block));
    GPUres_init(&diskTextures);

    if(!texResult) {
        LOG_ERR("thread_game> pak_texturesRead() failed");
        return 0;
    }

    while(client.running) {
        GPUres_newFrame();
        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            texDiskIds[i] = i + (pageId * PAGE_TEXTURES_COUNT);
        }
        GPUres_requestTextures(texDiskIds, gpu_textures, PAGE_TEXTURES_COUNT);

        // NOTE: dont push render command inside UI code
#ifdef CONF_DEBUG
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            ui_textureBrowser();
            ui_videoInfo();
            //GPUres_debugUi();
            //ImGui::ShowTestWindow();
        }
        client.dbguiNewFrameEnd();
#endif

        CommandList list;
        list.queryVramInfo(&dedicated, &availMemory, &currentAvailMem, &evictionCount, &evictedMem);
        list.clear();
        list.endFrame();
        renderer_pushCommandList(list);
    }

    // TODO: wait for deinit
    GPUres_deinit();

    LOG("Game> initializated.");
    return 0;
}
