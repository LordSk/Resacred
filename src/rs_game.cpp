#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "rs_math.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_resources.h"



// interesting sectors:
/*
 * 130
 * 246
 * 250
 * 299
 * 1360
 * 1750
 * 1752
 * 2123
 * 4878
*/



#define PAGE_TEXTURES_COUNT 500
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
#define VIEW_X_ANGLE rs_radians(53.66563)

static i32 dedicated = 0, availMemory = 0, currentAvailMem = 0, evictionCount = 0, evictedMem = 0;

static vec2f tileUV[18][4];

void initTileUVs()
{
    tileUV[0][0] = vec2f(0, 24.5/256.f);
    tileUV[0][1] = vec2f(50.5/256.f, 0/256.f);
    tileUV[0][2] = vec2f(99.5/256.f, 24.5/256.f);
    tileUV[0][3] = vec2f(50.5/256.f, 49/256.f);

    tileUV[1][0] = vec2f(104.5/256.f, 24.5/256.f);
    tileUV[1][1] = vec2f(153.5/256.f, 0/256.f);
    tileUV[1][2] = vec2f(204/256.f, 24.5/256.f);
    tileUV[1][3] = vec2f(153.5/256.f, 49/256.f);

    for(i32 i = 2; i < 18; i += 2) {
        i32 line = i / 2;
        f32 offsetX = (line & 1) * 52/256.f;

        tileUV[i][0] = vec2fAdd(tileUV[0][0], vec2f(offsetX, 25/256.f * line));
        tileUV[i][1] = vec2fAdd(tileUV[0][1], vec2f(offsetX, 25/256.f * line));
        tileUV[i][2] = vec2fAdd(tileUV[0][2], vec2f(offsetX, 25/256.f * line));
        tileUV[i][3] = vec2fAdd(tileUV[0][3], vec2f(offsetX, 25/256.f * line));

        tileUV[i+1][0] = vec2fAdd(tileUV[1][0], vec2f(offsetX, 25/256.f * line));
        tileUV[i+1][1] = vec2fAdd(tileUV[1][1], vec2f(offsetX, 25/256.f * line));
        tileUV[i+1][2] = vec2fAdd(tileUV[1][2], vec2f(offsetX, 25/256.f * line));
        tileUV[i+1][3] = vec2fAdd(tileUV[1][3], vec2f(offsetX, 25/256.f * line));
    }
}

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

void makeTileMesh(TileVertex* mesh, i32 localTileId, f32 offsetX, f32 offsetY)
{
    assert(localTileId >= 0 && localTileId < 18);
    /*
    mesh[0] = TileVertex(0, 0.25,     tileUV[localTileId][0].x, tileUV[localTileId][0].y);
    mesh[1] = TileVertex(0.5, 0,      tileUV[localTileId][1].x, tileUV[localTileId][1].y);
    mesh[2] = TileVertex(1, 0.25,     tileUV[localTileId][2].x, tileUV[localTileId][2].y);
    mesh[3] = TileVertex(0, 0.25,     tileUV[localTileId][0].x, tileUV[localTileId][0].y);
    mesh[4] = TileVertex(1, 0.25,     tileUV[localTileId][2].x, tileUV[localTileId][2].y);
    mesh[5] = TileVertex(0.5, 0.5,    tileUV[localTileId][3].x, tileUV[localTileId][3].y);
    */

    /*
    mesh[0] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][0].x, tileUV[localTileId][0].y);
    mesh[1] = TileVertex(1.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][1].x, tileUV[localTileId][1].y);
    mesh[2] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][2].x, tileUV[localTileId][2].y);
    mesh[3] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][0].x, tileUV[localTileId][0].y);
    mesh[4] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][2].x, tileUV[localTileId][2].y);
    mesh[5] = TileVertex(0.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][3].x, tileUV[localTileId][3].y);
    */

    mesh[0] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][1].x, tileUV[localTileId][1].y);
    mesh[1] = TileVertex(1.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][2].x, tileUV[localTileId][2].y);
    mesh[2] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][3].x, tileUV[localTileId][3].y);
    mesh[3] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][1].x, tileUV[localTileId][1].y);
    mesh[4] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][3].x, tileUV[localTileId][3].y);
    mesh[5] = TileVertex(0.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][0].x, tileUV[localTileId][0].y);
}

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
    i32 texDiskIds[PAGE_TEXTURES_COUNT];
    GLuint* gpu_textures[PAGE_TEXTURES_COUNT];
    i32 pageId = 0;
    i32 testTileLocalId = 0;
    TileVertex tileVertexData[6 * 64 * 64];
    MutexSpin tileVertexMutex;

    TileShader tileShader;

    bool viewIsIso = true;
    mat4 viewIso;
    mat4 testTileModel;
    f32 viewX = 0;
    f32 viewY = -500;
    f32 viewZoom = 1.0f;
    i32 viewZMul = -1;
    MutexSpin viewMutex;

    i32 dbgTileDrawCount = 4096;
    i32 dbgSectorId = 130;

    struct {
        u8 mouseLeft = 0;
        u8 mouseRight = 0;

        i32 mouseRelX = 0;
        i32 mouseRelY = 0;
        i32 mouseWheelRelY = 0;
    } input;

    void loadShaders()
    {
        tileShader.loadAndCompile();

        CommandList list;
        list.bufferData(&tileShader.vertexBuffer, 0, sizeof(tileVertexData), GL_STATIC_DRAW);
        renderer_pushCommandList(list);
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
        /*memset(gpu_textures, 0, sizeof(gpu_textures));
        bool texResult = pak_texturesRead("../sacred_data/texture.pak", &diskTextures);
        assert(texResult);
        GPUres_init(&diskTextures);*/
    }

    void requestTexBrowserTextures()
    {
        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            texDiskIds[i] = i + (pageId * PAGE_TEXTURES_COUNT);
        }
        resource_requestGpuTextures(texDiskIds, gpu_textures, PAGE_TEXTURES_COUNT);
    }

    void ui_textureBrowser()
    {
#if 0
        ImGui::Begin("Textures");
        ImGui::SliderInt("page", &pageId, 0, 3000/PAGE_TEXTURES_COUNT - 1);

        ImGui::BeginChild("texture_list");

        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            ImGui::BeginGroup();
            ImGui::Image((ImTextureID)(intptr_t)*gpu_textures[i],
                         ImVec2(256, 256));
            i32 diskTexId = texDiskIds[i];
            ImGui::Text("%s (%d/%d) #%d", diskTextures.textureName[diskTexId],
                        diskTextures.textureInfo[diskTexId].width,
                        diskTextures.textureInfo[diskTexId].height,
                        diskTexId);
            ImGui::EndGroup();

            if((i + 1)%4 != 0) {
                ImGui::SameLine();
            }
        }
        ImGui::EndChild();

        ImGui::End();
#endif
    }

    void ui_tileTest()
    {
        ImGui::Begin("Test tile");

        ImGui::SliderInt("viewZMul", &viewZMul, -8, 8);
        ImGui::Checkbox("Isometric view", &viewIsIso);
        ImGui::SliderInt("dbgTileDrawCount", &dbgTileDrawCount, 1, 4096);


        if(ImGui::Button("+")) dbgSectorId++;
        ImGui::SameLine();
        ImGui::SliderInt("dbgSectorId", &dbgSectorId, 1, 6049);
        ImGui::SameLine();
        if(ImGui::Button("-")) dbgSectorId--;

        const DiskSectors::Sector& sector = diskSectors.sectors[dbgSectorId];

        ImGui::Text("Sector %d:", sector.id);
        ImGui::Text("posX1=%d posX2=%d posY1=%d posY2=%d",
                    sector.posX1, sector.posX2, sector.posY1, sector.posY2);

        ImGui::Image((ImTextureID)(intptr_t)resource_defaultGpuTexture(), ImVec2(256, 256));

        ImGui::End();
    }

    void drawTestTiles()
    {
        constexpr i32 MAX_SECTOR_TEXTURE_COUNT = 200;
        i32 diskSectorTexs[MAX_SECTOR_TEXTURE_COUNT] = {};
        GLuint* gpuSectorTexs[MAX_SECTOR_TEXTURE_COUNT];
        i32 sectorTextureCount = 0;
        GLuint* tileGpuTexId[4097];

        for(i32 i = 1; i < 4097; ++i) {
            i32 texId = diskTiles.tiles[diskSectors.sectors[dbgSectorId].wldxEntries[i].tileId].textureId;
            //assert(texId >= 0 && texId < diskTextures.textureCount);

            bool found = false;
            for(i32 j = 0; j < sectorTextureCount && !found; ++j) {
                if(diskSectorTexs[j] == texId) {
                    found = true;
                }
            }

            if(!found) {
                diskSectorTexs[sectorTextureCount++] = texId;
                assert(sectorTextureCount <= MAX_SECTOR_TEXTURE_COUNT);
            }
        }

        resource_requestGpuTextures(diskSectorTexs, gpuSectorTexs, sectorTextureCount);

        // assign gpu textures to tiles
        for(i32 i = 1; i < 4097; ++i) {
            i32 texId = diskTiles.tiles[diskSectors.sectors[dbgSectorId].wldxEntries[i].tileId].textureId;

            for(i32 j = 0; j < sectorTextureCount; ++j) {
                if(diskSectorTexs[j] == texId) {
                    tileGpuTexId[i] = gpuSectorTexs[j];
                }
            }
        }

        CommandList list;
        list.useProgram(&tileShader.program);

        viewMutex.lock();
        viewIso = mat4Orthographic(0, 1600 * viewZoom, 900 * viewZoom, 0, -10000.f, 10000.f);
        viewIso = mat4Mul(viewIso, mat4Translate(vec3f(-viewX, -viewY, 0)));
        if(viewIsIso) {
            viewIso = mat4Mul(viewIso, mat4RotateAxisX(VIEW_X_ANGLE));
            viewIso = mat4Mul(viewIso, mat4RotateAxisZ(RS_HALFPI/2 * viewZMul));
        }
        viewMutex.unlock();

        list.lock(&viewMutex);
        list.uniformMat4(tileShader.uViewMatrix, &viewIso);
        list.unlock(&viewMutex);

        testTileModel = mat4Scale(vec3f(53.65625, 53.65625, 1));
        list.uniformMat4(tileShader.uModelMatrix, &testTileModel);

        static i32 slot = 0;
        list.uniformInt(tileShader.uDiffuse, &slot);

        tileVertexMutex.lock();
        i32 tileMeshId = 0;
        for(i32 y = 0; y < 64; ++y) {
            for(i32 x = 0; x < 64; ++x) {
                DiskSectors::WldxEntry& we = diskSectors.sectors[dbgSectorId].wldxEntries[y*64 + x + 1];
                makeTileMesh(&tileVertexData[6 * (tileMeshId++)], we.tileId % 18, x, y);
            }
        }
        tileVertexMutex.unlock();

        list.lock(&tileVertexMutex);
        list.arrayBufferSubData(&tileShader.vertexBuffer, 0, tileVertexData, sizeof(tileVertexData));
        list.unlock(&tileVertexMutex);


        list.bindVertexArray(&tileShader.vao);

        /*i32 currentDiskTexId = 0;
        i32 drawQueueFirst = 0;
        i32 drawQueueCount = 0;
        for(i32 i = 0; i < dbgTileDrawCount; ++i) {
            i32 texId = diskTiles.tiles[diskSectors.sectors[1].wldxEntries[i+1].tileId].textureId;

            if(texId != currentDiskTexId) {
                list.textureSlot(tileGpuTexId[i+1], 0);
                list.drawTriangles(drawQueueFirst * 6, drawQueueCount * 6);
                drawQueueFirst = i;
                drawQueueCount = 1;
            }
            else {
                drawQueueCount++;
            }
        }

        list.drawTriangles(drawQueueFirst, drawQueueCount * 6);*/

        // TODO: pack same texture calls or bind more textures
        for(i32 i = 0; i < dbgTileDrawCount; ++i) {
            list.textureSlot(tileGpuTexId[i+1], 0);
            list.drawTriangles(i * 6, 6);
        }

        renderer_pushCommandList(list);
    }

    // NOTE: async (coming from another thread)
    // TODO: mutex?
    void receiveInput(const SDL_Event& event)
    {
        if(event.type == SDL_MOUSEBUTTONDOWN) {
            input.mouseLeft |= event.button.button == SDL_BUTTON_LEFT;
            input.mouseRight |= event.button.button == SDL_BUTTON_RIGHT;
        }
        else if(event.type == SDL_MOUSEBUTTONUP) {
            input.mouseLeft &= !(event.button.button == SDL_BUTTON_LEFT);
            input.mouseRight &= !(event.button.button == SDL_BUTTON_RIGHT);
        }
        else if(event.type == SDL_MOUSEMOTION) {
            input.mouseRelX += event.motion.xrel;
            input.mouseRelY += event.motion.yrel;
        }
        else if(event.type == SDL_MOUSEWHEEL) {
            input.mouseWheelRelY += event.wheel.y;
        }
    }

    void processInput()
    {
        if(input.mouseWheelRelY < 0) {
            viewZoom *= 1.10f;
        }
        else if(input.mouseWheelRelY > 0) {
            viewZoom *= 0.9f;
        }

        if(input.mouseRight) {
            viewX -= input.mouseRelX * viewZoom;
            viewY -= input.mouseRelY * viewZoom;
        }

        input.mouseRelX = 0;
        input.mouseRelY = 0;
        input.mouseWheelRelY = 0;
    }

    void deinit()
    {
        MEM_DEALLOC(diskTiles.block);
        MEM_DEALLOC(diskSectors.block);
    }
};

static Game* pGame = nullptr;

static void receiveGameInput(const SDL_Event& event, void* userData)
{
    Game& game = *(Game*)userData;
    game.receiveInput(event);
}

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
    LOG("thread_game started");

    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    initTileUVs();

    renderer_waitForInit();
    client.dbguiWaitForInit();

    Game game;
    pGame = &game;

    client.addInputCallback(receiveGameInput, &game);
    game.loadShaders();

    //game.loadTextures();
    game.loadSectors();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    LOG_SUCC("Game> initializated");

    while(client.running) {
        game.processInput();

        resource_newFrame();
        //game.requestTexBrowserTextures();

        // NOTE: dont push render command inside UI code
#ifdef CONF_DEBUG
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            //game.ui_textureBrowser();
            game.ui_tileTest();
            ui_videoInfo();
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
    LOG("Game> cleaning up...");
    game.deinit();

    return 0;
}
