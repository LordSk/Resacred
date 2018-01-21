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
 * 690
 * 1360
 * 1750
 * 1752
 * 2123
 * 4878
 * 5351
*/

// interesting texture page: 27 (minimap)

#define PAGE_TEXTURES_COUNT 160
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
    u32 color;

    TileVertex() = default;
    TileVertex(f32 _x, f32 _y, f32 _u, f32 _v, u32 _c) {
        x = _x;
        y = _y;
        u = _u;
        v = _v;
        color = _c;
    }
};

void makeTileMesh(TileVertex* mesh, i32 localTileId, f32 offsetX, f32 offsetY, u32 color)
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

    mesh[0] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][1].x, tileUV[localTileId][1].y, color);
    mesh[1] = TileVertex(1.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][2].x, tileUV[localTileId][2].y, color);
    mesh[2] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][3].x, tileUV[localTileId][3].y, color);
    mesh[3] = TileVertex(0.0 + offsetX, 0.0 + offsetY, tileUV[localTileId][1].x, tileUV[localTileId][1].y, color);
    mesh[4] = TileVertex(1.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][3].x, tileUV[localTileId][3].y, color);
    mesh[5] = TileVertex(0.0 + offsetX, 1.0 + offsetY, tileUV[localTileId][0].x, tileUV[localTileId][0].y, color);
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
            layout(location = 2) in vec4 color;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec2 vert_uv;
            out vec4 vert_color;

            void main()
            {
                vert_uv = uv;
                vert_color = color;
                gl_Position = uViewMatrix * uModelMatrix * vec4(position, 0.0, 1.0);
            }
            )FOO";

        constexpr const char* fragmentShader = R"FOO(
            #version 330 core
            uniform sampler2D uDiffuse;

            in vec2 vert_uv;
            in vec4 vert_color;
            out vec4 fragmentColor;

            void main()
            {
                fragmentColor = texture(uDiffuse, vert_uv) * vert_color;
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
            COLOR = 2,
        };

        i32 indexes[] = {POSITION, UV, COLOR};
        list.enableVertexAttribArrays(indexes, sizeof(indexes)/sizeof(Location));

        list.vertexAttribPointer(Location::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, x));
        list.vertexAttribPointer(Location::UV, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, u));
        list.vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, color));

        RBarrier barrierShader;
        list.barrier(&barrierShader, "Tile shader");
        list.execute();
        renderer_pushCommandList(list);

        renderer_waitForBarrier(&barrierShader);
    }
};

struct Game
{
    i32 texBrowser_texIds[PAGE_TEXTURES_COUNT];
    GLuint* texBrowser_texGpu[PAGE_TEXTURES_COUNT];
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
    i32 dbgSectorId = 690;
    i32 loadedSectorId = -1;
    WldxEntry* sectorData = nullptr;

    enum {
        MODE_NORMAL = 0,
        MODE_STATIC,
        MODE_ENTITY,
        MODE_SMTH_POS,
        MODE_SMTH_TYPE,
        MODE_COUNT
    };

    const char* viewModeCombo[MODE_COUNT] = {
        "NORMAL",
        "STATIC",
        "ENTITY",
        "SMTH_POS",
        "SMTH_TYPE",
    };
    i32 dbgViewMode = MODE_NORMAL;

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

    void loadSectorIfNeeded()
    {
        if(loadedSectorId != dbgSectorId) {
            sectorData = resource_loadSector(dbgSectorId);
            loadedSectorId = dbgSectorId;
        }
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
            texBrowser_texIds[i] = i + (pageId * PAGE_TEXTURES_COUNT) + 1;
        }
        resource_requestGpuTextures(texBrowser_texIds, texBrowser_texGpu, PAGE_TEXTURES_COUNT);
    }

    void ui_textureBrowser()
    {
#if 1
        ImGui::Begin("Textures");
        ImGui::SliderInt("page", &pageId, 0, resource_getTextureCount()/PAGE_TEXTURES_COUNT - 1);

        PakTextureInfo* texInfo = resource_getTextureInfos();
        ImGui::BeginChild("texture_list");

        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            ImGui::BeginGroup();
            ImGui::Image((ImTextureID)(intptr_t)*texBrowser_texGpu[i],
                         ImVec2(256, 256));
            i32 diskTexId = texBrowser_texIds[i];

#ifdef CONF_DEBUG
            ImGui::Text("%s (%d/%d) #%d", texInfo[diskTexId].name,
                        texInfo[diskTexId].width,
                        texInfo[diskTexId].height,
                        diskTexId);
#endif

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
        ImGui::Begin("Sector viewer");

        //ImGui::SliderInt("viewZMul", &viewZMul, -8, 8);
        ImGui::Checkbox("Isometric view", &viewIsIso);
        ImGui::Combo("viewMode", &dbgViewMode, viewModeCombo, MODE_COUNT);

        ImGui::SliderInt("dbgTileDrawCount", &dbgTileDrawCount, 1, 4096);


        ImGui::SliderInt("dbgSectorId", &dbgSectorId, 1, 6049);
        ImGui::SameLine();
        if(ImGui::Button("-")) dbgSectorId--;
        ImGui::SameLine();
        if(ImGui::Button("+")) dbgSectorId++;


        /*ImGui::Text("Sector %d:", sector.id);
        ImGui::Text("posX1=%d posX2=%d posY1=%d posY2=%d count=%d",
                    sector.posX1, sector.posX2, sector.posY1, sector.posY2,
                    sector.wldxEntryCount);*/

        //ImGui::Image((ImTextureID)(intptr_t)resource_defaultGpuTexture(), ImVec2(256, 256));

        ImGui::End();
    }

    void drawTestTiles()
    {
        constexpr i32 MAX_SECTOR_TEXTURE_COUNT = 300;
        constexpr i32 MAX_TILE_ENTRIES = 4097;
        i32 diskSectorTexs[MAX_SECTOR_TEXTURE_COUNT] = {};
        GLuint* gpuSectorTexs[MAX_SECTOR_TEXTURE_COUNT];
        i32 sectorTextureCount = 0;
        GLuint* tileGpuTexId[MAX_TILE_ENTRIES];

        u16* tileTexIds = resource_getTileTextureIds18();

        for(i32 i = 1; i < MAX_TILE_ENTRIES; ++i) {
            i32 tileId = sectorData[i].tileId;
            i32 texId = tileTexIds[tileId/18];

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
        for(i32 i = 1; i < MAX_TILE_ENTRIES; ++i) {
            i32 tileId = sectorData[i].tileId;
            i32 texId = tileTexIds[tileId/18];

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
                WldxEntry& we = sectorData[y*64 + x + 1];
                u32 color = 0xff000000;

                switch(dbgViewMode) {
                    case MODE_NORMAL:
                        color = 0xffffffff;
                        break;

                    case MODE_STATIC:
                        if(we.staticId) {
                            color = 0xff0000ff;
                        }
                        break;

                    case MODE_ENTITY:
                        if(we.entityId) {
                            color = 0xffff0000;
                        }
                        break;

                    case MODE_SMTH_TYPE:
                        if(we.someTypeId) {
                            color = 0xff000000 | (0x00000001 * (we.someTypeId*(255/15)));
                        }
                        break;

                    case MODE_SMTH_POS:
                        color = 0xff000000 | (we.smthZ << 16) | (we.smthY << 8) | (we.smthX);
                        break;
                }
                makeTileMesh(&tileVertexData[6 * (tileMeshId++)], we.tileId % 18, x, y, color);
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
            i32 texId = tileTexIds[diskSectors.sectors[1].wldxEntries[i+1].tileId].textureId;

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
    resource_init();

    Game game;
    pGame = &game;

    client.addInputCallback(receiveGameInput, &game);
    game.loadShaders();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    LOG_SUCC("Game> initializated");

    while(client.running) {
        game.processInput();
        resource_newFrame();

        game.requestTexBrowserTextures();

        // NOTE: dont push render command inside UI code
#ifdef CONF_DEBUG
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            game.ui_textureBrowser();
            game.ui_tileTest();
            ui_videoInfo();
            //ImGui::ShowTestWindow();
        }
        client.dbguiNewFrameEnd();
#endif

        game.loadSectorIfNeeded();

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
