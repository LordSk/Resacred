#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "rs_math.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_gpu_resources.h"


#define PAGE_TEXTURES_COUNT 500
static i32 dedicated = 0, availMemory = 0, currentAvailMem = 0, evictionCount = 0, evictedMem = 0;

static const vec2f tileUV[18][4] = {
    // 0
    { vec2f(0, 24/256.f), vec2f(50/256.f, -1/256.f),
      vec2f(100/256.f, 24/256.f), vec2f(50/256.f, 50/256.f) },

    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 1
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 2
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 3
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 4
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 5
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 6
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 7
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 8
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 9
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 10
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 11
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 12
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 13
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 14
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 15
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }, // 16
    { vec2f(0, 0), vec2f(0, 0), vec2f(0, 0), vec2f(0, 0) }  // 17
};

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

struct TileVertex
{
    f32 x, y;
    f32 u, v;

    TileVertex() = default;
    TileVertex(f32 _x, f32 _y, f32 _u, f32 _v) {
        x = _x;
        y = _y;
        u = _u;
        v = _v;
    }
};

struct TileShader
{
    GLuint program;
    i32 uViewMatrix;
    i32 uModelMatrix;
    i32 uDiffuse;
    GLuint vao;
    GLuint vertexBuffer;

    void loadAndCompile()
    {
        // ui shader
        constexpr const char* vertexShader = R"FOO(
            #version 330 core
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec2 uv;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec2 vert_uv;

            void main()
            {
                vert_uv = uv;
                gl_Position = uViewMatrix * uModelMatrix * vec4(position, 0.0, 1.0);
            }
            )FOO";

        constexpr const char* fragmentShader = R"FOO(
            #version 330 core
            uniform sampler2D uDiffuse;

            in vec2 vert_uv;
            out vec4 fragmentColor;

            void main()
            {
                fragmentColor = texture(uDiffuse, vert_uv);
            }
            )FOO";

        u32 vertexShaderLen = strlen(vertexShader);
        u32 fragmentShaderLen = strlen(fragmentShader);

        MemBlock shaderBuffers[2];
        shaderBuffers[0].ptr = (void*)vertexShader;
        shaderBuffers[0].size = vertexShaderLen;
        shaderBuffers[1].ptr = (void*)fragmentShader;
        shaderBuffers[1].size = fragmentShaderLen;

        const i32 types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };

        CommandList list;
        list.createShaderAndCompile(shaderBuffers, types, 2, &program);

        i32* locations[] = {&uViewMatrix, &uDiffuse, &uModelMatrix};
        const char* uniformNames[] = {"uViewMatrix", "uDiffuse", "uModelMatrix"};
        list.getUniformLocations(&program, locations, uniformNames, sizeof(locations)/sizeof(locations[0]));

        list.genBuffers(&vertexBuffer, 1);
        list.genVertexArrays(&vao, 1);
        list.bindVertexArray(&vao);
        list.bindArrayBuffer(&vertexBuffer);

        enum Location {
            POSITION = 0,
            UV = 1,
        };

        i32 indexes[] = {POSITION, UV};
        list.enableVertexAttribArrays(indexes, sizeof(indexes)/sizeof(Location));

        list.vertexAttribPointer(Location::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, x));
        list.vertexAttribPointer(Location::UV, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, u));

        RBarrier barrierShader;
        list.barrier(&barrierShader, "Tile shader");
        list.execute();
        renderer_pushCommandList(list);

        renderer_waitForBarrier(&barrierShader);
    }
};

struct Game
{
    DiskTiles diskTiles;
    DiskSectors diskSectors;
    DiskTextures diskTextures;
    i32 texDiskIds[PAGE_TEXTURES_COUNT];
    GLuint* gpu_textures[PAGE_TEXTURES_COUNT];
    i32 pageId = 0;

    TileShader tileShader;

    void loadShaders()
    {
        tileShader.loadAndCompile();
    }

    void loadSectors()
    {
        pak_tilesRead("../sacred_data/tiles.pak", &diskTiles);
        keyx_sectorsRead("../sacred_data/sectors.keyx", "../sacred_data/sectors.wldx", &diskSectors);

        // const i32 sectorCount = diskSectors.sectors.count();
        /*const i32 sectorCount = 2;
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
        }*/
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

    void uploadTestTileData()
    {
        static const TileVertex testTileData[] = {
            TileVertex(0, 0.25, tileUV[0][0].x, tileUV[0][0].y),
            TileVertex(0.5, 0, tileUV[0][1].x, tileUV[0][1].y),
            TileVertex(1, 0.25, tileUV[0][2].x, tileUV[0][2].y),
            TileVertex(0, 0.25, tileUV[0][0].x, tileUV[0][0].y),
            TileVertex(1, 0.25, tileUV[0][2].x, tileUV[0][2].y),
            TileVertex(0.5, 0.5, tileUV[0][3].x, tileUV[0][3].y)
        };

        CommandList list;
        list.arrayBufferData(&tileShader.vertexBuffer, testTileData, sizeof(testTileData), GL_STATIC_DRAW);

        list.useProgram(&tileShader.program);
        static const mat4 orth = mat4Orthographic(0, 1600, 900, 0, -1.f, 1.f);
        list.uniformMat4(tileShader.uViewMatrix, &orth);
        static const mat4 scale = mat4Scale(vec3f(500, 500, 1));
        list.uniformMat4(tileShader.uModelMatrix, &scale);

        renderer_pushCommandList(list);
    }

    void drawTestTiles()
    {
        GLuint* gpuTexture;
        i32 tileTexId = diskTiles.tiles[diskSectors.sectors[1].wldxEntries[1].tileId].textureId;
        GPUres_requestTextures(&tileTexId, &gpuTexture, 1);

        CommandList list;
        list.useProgram(&tileShader.program);

        static i32 slot = 0;
        list.textureSlot(gpuTexture, 0);
        list.uniformInt(tileShader.uDiffuse, &slot);
        list.bindVertexArray(&tileShader.vao);

        list.drawTriangles(0, 6);
        renderer_pushCommandList(list);
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
    game.loadTextures();
    game.loadSectors();

    game.loadShaders();
    game.uploadTestTileData();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    while(client.running) {
        GPUres_newFrame();
        game.requestTextures();

        // NOTE: dont push render command inside UI code
#ifdef CONF_DEBUG
        client.dbguiNewFrameBegin();
        if(client.imguiSetup && 0) {
            game.ui_textureBrowser();
            ui_videoInfo();
            //GPUres_debugUi();
            //ImGui::ShowTestWindow();
        }
        client.dbguiNewFrameEnd();
#endif
        CommandList list;
        list.clear();
        renderer_pushCommandList(list);

        game.drawTestTiles();

        list = {};
        list.queryVramInfo(&dedicated, &availMemory, &currentAvailMem, &evictionCount, &evictedMem);
        list.endFrame();
        renderer_pushCommandList(list);
    }

    // TODO: wait for deinit
    GPUres_deinit();

    LOG("Game> initializated.");
    return 0;
}
