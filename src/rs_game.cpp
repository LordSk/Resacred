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

i32 thread_game(void* data)
{
    LOG("Game> initialization...");

    //pak_tilesRead("../sacred_data/tiles.pak", nullptr);
    DiskTextures diskTextures;
    assert(pak_texturesRead("../sacred_data/texture.pak", &diskTextures));
    defer(MEM_DEALLOC(diskTextures.block));

    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    const i32 VIEW_ID_OFFSET = 2000;

    // upload textures to gpu
    CommandList cmds;
    for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
        i32 id = i + VIEW_ID_OFFSET;
        texDescs[i].width = diskTextures.textureInfo[id].width;
        texDescs[i].height = diskTextures.textureInfo[id].height;
        texDescs[i].data =  diskTextures.textureData[id];

        if(diskTextures.textureInfo[id].type == 6) {
            texDescs[i].internalFormat = GL_RGBA8;
            texDescs[i].dataFormat = GL_RGBA;
            texDescs[i].dataPixelCompType = GL_UNSIGNED_BYTE;
        }
        else {
            texDescs[i].internalFormat = GL_RGBA8;
            texDescs[i].dataFormat = GL_BGRA;
            texDescs[i].dataPixelCompType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        }

        cmds.createTexture2D(&texDescs[i], &gpu_textures[i]);
    }
    cmds.execute();
    renderer_pushCommandList(cmds);

    while(1) {
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            ImGui::Begin("Textures");
            for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
                ImGui::Image((ImTextureID)(intptr_t)gpu_textures[i],
                             ImVec2(texDescs[i].width, texDescs[i].height));
                ImGui::SameLine();
                ImGui::Text("%32s %d", diskTextures.textureName[i + VIEW_ID_OFFSET].data,
                        diskTextures.textureInfo[i + VIEW_ID_OFFSET].type);
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
