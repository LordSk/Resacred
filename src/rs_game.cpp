#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_gpu_resources.h"


#define PAGE_TEXTURES_COUNT 500
static i32 dedicated = 0, availMemory = 0, currentAvailMem = 0, evictionCount = 0, evictedMem = 0;

struct Game
{
    DiskTiles diskTiles;
    DiskSectors diskSectors;
    DiskTextures diskTextures;
    i32 texDiskIds[PAGE_TEXTURES_COUNT];
    GLuint* gpu_textures[PAGE_TEXTURES_COUNT];
    i32 pageId = 0;

    void loadSectors()
    {
        pak_tilesRead("../sacred_data/tiles.pak", &diskTiles);
        keyx_sectorsRead("../sacred_data/sectors.keyx", "../sacred_data/sectors.wldx", &diskSectors);

        // const i32 sectorCount = diskSectors.sectors.count();
        const i32 sectorCount = 2;
        const DiskSectors::Sector* sectors = diskSectors.sectors.data();
        for(i32 s = 1; s < sectorCount; ++s) {
            LOG_DBG("Sector#%d", s);

            const i32 wldxEntryCount = sectors[s].wldxEntryCount;
            for(i32 w = 1; w < wldxEntryCount; ++w) {
                DiskSectors::WldxEntry& we = sectors[s].wldxEntries[w];
                LOG_DBG("[%d] tileId=%d texId=%d static=%d entity=%d px=%d py=%d [%d, %d, %d, %d] %d", w,
                        we.tileId,
                        diskTiles.tiles[we.tileId].textureId,
                        we.staticId,
                        we.entityId,
                        we.posInfo & (64-1),
                        we.posInfo / 64,
                        we.smthX, we.smthY, we.smthZ, we.smthW,
                        we.someTypeId & 0xF);
            }
        }
    }

    void loadTextures()
    {
        memset(gpu_textures, 0, sizeof(gpu_textures));
        bool texResult = pak_texturesRead("../sacred_data/texture.pak", &diskTextures);
        assert(texResult);
        GPUres_init(&diskTextures);
    }

    void requestTextures()
    {
        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            texDiskIds[i] = i + (pageId * PAGE_TEXTURES_COUNT);
        }
        GPUres_requestTextures(texDiskIds, gpu_textures, PAGE_TEXTURES_COUNT);
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

    void deinit()
    {
        MEM_DEALLOC(diskTextures.block);
        MEM_DEALLOC(diskTiles.block);
        MEM_DEALLOC(diskSectors.block);
    }
};

static Game* pGame = nullptr;

void ui_videoInfo()
{
    ImGui::Begin("Video info");

    ImGui::Text("Total available: %dkb", availMemory);
    ImGui::Text("Current Avilable: %dkb", currentAvailMem);
    ImGui::ProgressBar(1.0 - (currentAvailMem / (f64)availMemory));

    ImGui::End();
}

i32 thread_game(void*)
{
    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    Game game;
    pGame = &game;
    game.loadSectors();
    game.loadTextures();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    while(client.running) {
        GPUres_newFrame();
        game.requestTextures();

        // NOTE: dont push render command inside UI code
#ifdef CONF_DEBUG
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            game.ui_textureBrowser();
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
