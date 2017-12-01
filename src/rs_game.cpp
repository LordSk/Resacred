#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "imgui.h"
#include "stb_image.h"
#include "zlib.h"

#define GPU_TEXTURES_COUNT 500
static GLuint gpu_textures[GPU_TEXTURES_COUNT];
static TextureDesc2D texDescs[GPU_TEXTURES_COUNT];


/*
TODO:
- Investigate texture type 4

*/

i32 decompress(const char* input, const i32 inputSize, u8* output)
{
    constexpr i32 CHUNK = 16384;
    i32 ret;
    u32 have;
    z_stream strm;
    u8 in[CHUNK];
    u8 out[CHUNK];
    i32 inCursor = 0;
    i32 outCursor = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if(ret != Z_OK) {
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        i32 copySize = CHUNK;
        if(inputSize - inCursor < CHUNK) {
            copySize = inputSize - inCursor;
        }
        strm.avail_in = copySize;
        if(copySize <= 0) {
            break;
        }
        memmove(in, input + inCursor, copySize);
        inCursor += copySize;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            memmove(output + outCursor, out, have);
            outCursor += have;
            /*if(outCursor > outputMaxSize) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }*/
        } while(strm.avail_out == 0);
    } while(ret != Z_STREAM_END);

    /* clean up and return */
    inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

i32 thread_game(void* data)
{
    LOG("Game> initialization...");

    //pak_tilesRead("../sacred_data/tiles.pak", nullptr);
    PakTexture* pakTextures;
    assert(pak_texturesRead("../sacred_data/texture.pak", &pakTextures));

    Window& client = *get_clientWindow();
    renderer_waitForInit();
    client.dbguiWaitForInit();

    MemBlock texBlock = MEM_ALLOC(Megabyte(500));
    assert(texBlock.ptr);
    defer(MEM_DEALLOC(texBlock));
    u32 texBlockCursor = 0;
    u8* texBlockData = (u8*)texBlock.ptr;

    const i32 VIEW_ID_OFFSET = 1000;

    // upload textures to gpu
    CommandList cmds;
    for(int i = 0; i < GPU_TEXTURES_COUNT; ++i) {
        i32 id = i + VIEW_ID_OFFSET;
        texDescs[i].width = pakTextures[id].width;
        texDescs[i].height = pakTextures[id].height;
        texDescs[i].data =  pakTextures[id].data;

        if(pakTextures[id].typeId == 6) {
            texDescs[i].internalFormat = GL_RGBA8;
            texDescs[i].dataFormat = GL_RGBA;
            texDescs[i].dataPixelCompType = GL_UNSIGNED_BYTE;
        }
        else {
            i32 compressedSize = pakTextures[id+1].offset - pakTextures[id].offset - 80;
            if(compressedSize <= 0) {
                continue;
            }

            u8* output = texBlockData + texBlockCursor;
            i32 outputSize = texDescs[i].width * texDescs[i].height * 2;
            texBlockCursor += outputSize;
            i32 ret = decompress((const char*)pakTextures[id].data, compressedSize, output);
            assert(ret == Z_OK);

            texDescs[i].internalFormat = GL_RGBA8;
            texDescs[i].dataFormat = GL_BGRA;
            texDescs[i].dataPixelCompType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            texDescs[i].data =  output;
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
                ImGui::Text("%32s %d", pakTextures[i + VIEW_ID_OFFSET].filename,
                        pakTextures[i + VIEW_ID_OFFSET].typeId);
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
