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
 * 4859
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

void meshAddQuad(TileVertex* mesh, f32 x, f32 y, f32 width, f32 height, f32 uvX1, f32 uvY1, f32 uvX2, f32 uvY2,
                 u32 color)
{
    mesh[0] = TileVertex(x        , y         , uvX1, uvY1, color);
    mesh[1] = TileVertex(x + width, y         , uvX2, uvY1, color);
    mesh[2] = TileVertex(x + width, y + height, uvX2, uvY2, color);
    mesh[3] = TileVertex(x        , y         , uvX1, uvY1, color);
    mesh[4] = TileVertex(x + width, y + height, uvX2, uvY2, color);
    mesh[5] = TileVertex(x        , y + height, uvX1, uvY2, color);
}

struct TileShader
{
    GLuint program;
    i32 uViewMatrix;
    i32 uModelMatrix;
    i32 uDiffuse;
    GLuint vao;
    GLuint vbo;

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

        static MemBlock shaderBuffers[2];
        shaderBuffers[0].ptr = (void*)vertexShader;
        shaderBuffers[0].size = vertexShaderLen;
        shaderBuffers[1].ptr = (void*)fragmentShader;
        shaderBuffers[1].size = fragmentShaderLen;

        constexpr i32 types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };

        CommandList list;
        list.createShaderAndCompile(shaderBuffers, types, 2, &program);

        static i32* locations[] = {&uViewMatrix, &uDiffuse, &uModelMatrix};
        static const char* uniformNames[] = {"uViewMatrix", "uDiffuse", "uModelMatrix"};
        list.getUniformLocations(&program, locations, uniformNames, sizeof(locations)/sizeof(locations[0]));

        list.genVertexArrays(&vao, 1);
        list.bindVertexArray(&vao);
        list.genBuffers(&vbo, 1);
        list.bindArrayBuffer(&vbo);

        enum Location {
            POSITION = 0,
            UV = 1,
            COLOR = 2,
        };

        static i32 indexes[] = {POSITION, UV, COLOR};
        list.enableVertexAttribArrays(indexes, sizeof(indexes)/sizeof(Location));

        list.vertexAttribPointer(Location::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, x));
        list.vertexAttribPointer(Location::UV, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, u));
        list.vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, color));
        renderer_pushCommandList(list);
    }
};


#define MAX_TILE_COUNT 4096

static TileVertex tileVertexData[6 * MAX_TILE_COUNT];
static TileVertex tileFloorVertexData[6 * MAX_TILE_COUNT];

struct Game
{
    f64 frameTime = 0;
    i32 texBrowser_texIds[PAGE_TEXTURES_COUNT];
    GLuint* texBrowser_texGpu[PAGE_TEXTURES_COUNT];
    i32 pageId = 0;
    i32 testTileLocalId = 0;
    MutexSpin tileVertexMutex;
    MutexSpin tileFloorVertexMutex;

    TileShader tileShader;
    GLuint vboBaseTileMesh;
    GLuint vboFloorTileMesh;

    bool viewIsIso = true;
    mat4 viewIso;
    mat4 testTileModel;
    f32 viewX = 0;
    f32 viewY = -500;
    f32 viewZoom = 1.0f;
    i32 viewZMul = -1;
    MutexSpin viewMutex;

    i32 dbgTileDrawCount = MAX_TILE_COUNT;
    i32 dbgSectorId = 4859;
    i32 loadedSectorId = -1;
    SectorxData* sectorData = nullptr;
    SectorInfo sectorInfo;

    i32 dbgFloorOffset = 0;
    bool dbgFloorOver = 0;
    i32 dbgOverlayFloor = 0;

    GLuint* gpuFloorTexs[MAX_TILE_COUNT];
    i32 floorTexCount = 0;

    i32 dbgMixedId = 0;

    enum {
        MODE_NORMAL = 0,
        MODE_STATIC,
        MODE_ENTITY,
        MODE_SMTH_POS,
        MODE_SMTH_TYPE,
        MODE_POS_INFO,
        MODE_COUNT
    };

    const char* viewModeCombo[MODE_COUNT] = {
        "NORMAL",
        "STATIC",
        "ENTITY",
        "SMTH_POS",
        "SMTH_TYPE",
        "MODE_POS_INFO",
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
        list.arrayBufferData(&tileShader.vbo, 0, sizeof(tileVertexData) + sizeof(tileFloorVertexData),
                             GL_DYNAMIC_DRAW);
        renderer_pushCommandList(list);
    }

    void loadSectorIfNeeded()
    {
        if(loadedSectorId != dbgSectorId) {
            sectorData = resource_loadSector(dbgSectorId);
            sectorInfo = resource_getSectorInfo(dbgSectorId);
            loadedSectorId = dbgSectorId;
        }
    }

    void requestTexBrowserTextures()
    {
        for(int i = 0; i < PAGE_TEXTURES_COUNT; ++i) {
            texBrowser_texIds[i] = i + (pageId * PAGE_TEXTURES_COUNT) + 1;
        }
        resource_requestGpuTextures(texBrowser_texIds, texBrowser_texGpu, PAGE_TEXTURES_COUNT);
    }

    void requestFloorTextures()
    {
        if(!sectorData) return;
        floorTexCount = 0;

        /*i32 diskSectorTexs[MAX_TILE_COUNT];

        FloorEntry* floors = resource_getFloors();
        const i32 floorCount = resource_getFloorCount();
        u16* tileTexIds = resource_getTileTextureIds18();
        WldxEntry* sectorEntries = sectorData->data;

        for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
            WldxEntry& we = sectorEntries[i];
            i32 tileId = sectorEntries[i].tileId;
            if(we.floorId && dbgOverlayFloor) {
                assert(we.floorId < floorCount);
                tileId = dbgOverlayFloor == 1 ?
                            floors[we.floorId].pakTileIds & 0x1FFFF : floors[we.floorId].pakTileIds >> 17;
                i32 texId = tileTexIds[tileId/18];
                diskSectorTexs[floorTexCount++] = texId;
                assert(floorTexCount <= MAX_TILE_COUNT);
            }
        }

        resource_requestGpuTextures(diskSectorTexs, gpuFloorTexs, floorTexCount);*/
    }

    void ui_textureBrowser()
    {
#if 1
        ImGui::Begin("Textures");
        ImGui::SliderInt("##page", &pageId, 0, resource_getTextureCount()/PAGE_TEXTURES_COUNT - 1);
        //ImGui::SameLine();

        static i32 searchTexId = 0;
        ImGui::InputInt("##page_search", &searchTexId);
        ImGui::SameLine();
        if(ImGui::Button("Go")) {
            searchTexId = clamp(searchTexId, 0, resource_getTextureCount());
            pageId = searchTexId/PAGE_TEXTURES_COUNT;
        }

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

        //ImGui::SliderInt("dbgTileDrawCount", &dbgTileDrawCount, 1, 4096);


        ImGui::SliderInt("Sector ID", &dbgSectorId, 1, 6049);
        ImGui::InputInt("##sector_id_input", &dbgSectorId);

        /*ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
        if(ImGui::Button("-")) dbgSectorId--;
        ImGui::SameLine();
        if(ImGui::Button("+")) dbgSectorId++;
        ImGui::PopStyleVar(1);*/

        dbgSectorId = clamp(dbgSectorId, 1, 6049);

        ImGui::SliderInt("Overlay floors", &dbgOverlayFloor, 0, 2);


        if(sectorData) {
            ImGui::Text("#%d (%s)", dbgSectorId, sectorData->name);
            /*ImGui::Text("posX1=%d posX2=%d posY1=%d posY2=%d count=%d",
                        sector.posX1, sector.posX2, sector.posY1, sector.posY2,
                        sector.wldxEntryCount);*/

            char headerName[64];
            sprintf(headerName, "Height data (%d)", sectorInfo.numHeights);
            if(ImGui::CollapsingHeader(headerName)) {
                for(i32 i = 0; i < sectorInfo.numHeights; ++i) {
                    sprintf(headerName, "Height #%d", i);
                    if(ImGui::CollapsingHeader(headerName)) {
                        ImGui::Text("offset=%d", sectorData->heightData[i].offsetData);
                    }
                }
            }

            if(ImGui::CollapsingHeader("Static")) {
                ImGui::BeginChild("static_entries"/*, ImVec2(0, 300)*/);

                const PakStatic* statics = resource_getStatic();
                const PakItemType* itemTypes = resource_getItemTypes();
                //const PakItemType* itemTypes = resource_getMixed();
                const i32 staticsCount = resource_getStaticCount();
                const i32 itemTypesCount = resource_getItemTypesCount();
                char staticIdStr[256];


                for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
                    WldxEntry& we = sectorData->data[i];
                    if(we.staticId) {
                        assert(we.staticId < staticsCount);

                        sprintf(staticIdStr, "#%d", we.staticId);
                        if(ImGui::CollapsingHeader(staticIdStr)) {
                            const PakStatic& s = statics[we.staticId];
                            assert(s.itemTypeId < itemTypesCount);

                            ImGui::Text("itemTypeId     : %d", s.itemTypeId);
                            ImGui::Text("name           : %s", itemTypes[s.itemTypeId].grnName);
                            ImGui::Text("mixedId        : %d", itemTypes[s.itemTypeId].mixedId);

                            ImGui::Separator();

                            ImGui::Text("field_8        : %d", s.field_8);
                            ImGui::Text("field_C        : %d", s.field_C);
                            ImGui::Text("field_E        : %d", s.field_E);
                            ImGui::Text("field_12       : %d", s.field_12);
                            ImGui::Text("unk_0          : %d", s.unk_0);
                            ImGui::Text("parentId       : %d", s.parentId);
                            ImGui::Text("anotherParentId: %d", s.anotherParentId);
                            ImGui::Text("nextStaticId   : %d", s.nextStaticId);
                            ImGui::Text("patchX         : %d", s.patchX);
                            ImGui::Text("patchY         : %d", s.patchY);
                            ImGui::Text("triggerId      : %d", s.triggerId);
                        }
                    }
                }

                ImGui::EndChild();
            }

            if(ImGui::Button("Dump data to file")) {
                char path[256];
                sprintf(path, "sector_data.%d", dbgSectorId);
                fileWriteBuffer(path, (const char*)sectorData, sectorInfo.uncompressedSize);
            }

            ImGui::BeginChild("floor_textures");

            for(i32 i = 0; i < floorTexCount; ++i) {
                ImGui::Image((ImTextureID)(intptr_t)*gpuFloorTexs[i], ImVec2(256, 256));
            }

            ImGui::EndChild();
        }

        //ImGui::Image((ImTextureID)(intptr_t)resource_defaultGpuTexture(), ImVec2(256, 256));

        ImGui::End();
    }

    void ui_floorTest()
    {
        const i32 floorCount = resource_getFloorCount();

        ImGui::Begin("Floor viewer");

        ImGui::Checkbox("Isometric view", &viewIsIso);
        ImGui::SliderInt("offset", &dbgFloorOffset, 0, floorCount-4096);
        ImGui::Checkbox("Second tile", &dbgFloorOver);

        ImGui::End();
    }

    void ui_mixedViewer()
    {
        const i32 descCount = resource_getMixedDescsCount();

        ImGui::Begin("Mixed viewer");

        ImGui::Checkbox("Isometric view", &viewIsIso);
        ImGui::SliderInt("id", &dbgMixedId, 0, descCount);
        ImGui::InputInt("##id_input", &dbgMixedId);
        ImGui::Text("%d", resource_getMixedDescs()[dbgMixedId].count);

        dbgMixedId = clamp(dbgMixedId, 0, descCount);

        ImGui::End();
    }

    void drawTestTiles()
    {
        static GLuint* gpuBaseTex[MAX_TILE_COUNT];
        static i32 baseTileIds[MAX_TILE_COUNT];
        static i32 baseTileTexIds[MAX_TILE_COUNT];

        static GLuint* gpuFloorTex[MAX_TILE_COUNT];
        static i32 floorTileIds[MAX_TILE_COUNT];
        static i32 floorTileTexIds[MAX_TILE_COUNT];
        static i32 floorPosIndex[MAX_TILE_COUNT];
        i32 floorCount = 0;

        const FloorEntry* floors = resource_getFloors();
        const i32 floorMaxCount = resource_getFloorCount();
        const u16* tileTexIds = resource_getTileTextureIds18();
        const i32 tileCount = resource_getTileCount18() * 18;
        const WldxEntry* sectorEntries = sectorData->data;

        for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
            const WldxEntry& we = sectorEntries[i];
            i32 tileId = sectorEntries[i].tileId;
            i32 texId = tileTexIds[tileId/18];
            baseTileIds[i] = tileId;
            baseTileTexIds[i] = texId;
            assert(tileId < tileCount);

            i32 floorId = we.floorId;
            while(floorId && dbgOverlayFloor) {
                assert(floorId < floorMaxCount);
                tileId = dbgOverlayFloor == 1 ?
                            (floors[floorId].pakTileIds & 0x1FFFF) : (floors[floorId].pakTileIds >> 17);

                /*LOG_DBG("%d %x tileId1=%d tileId2=%d", i,
                        floors[we.floorId].pakTileIds,
                        floors[we.floorId].pakTileIds & 0x1FFFF,
                        floors[we.floorId].pakTileIds >> 17);*/

                if(tileId > 0) {
                    assert(floorCount < MAX_TILE_COUNT);
                    i32 fid = floorCount++;
                    floorTileIds[fid] = tileId;
                    floorTileTexIds[fid] = tileTexIds[tileId/18];
                    floorPosIndex[fid] = i;
                }
                floorId = floors[floorId].nextFloorId;
            }
        }

        resource_requestGpuTextures(baseTileTexIds, gpuBaseTex, MAX_TILE_COUNT);
        resource_requestGpuTextures(floorTileTexIds, gpuFloorTex, floorCount);

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

        list.mutexLock(&viewMutex);
        list.uniformMat4(tileShader.uViewMatrix, &viewIso);
        list.mutexUnlock(&viewMutex);

        testTileModel = mat4Scale(vec3f(53.65625, 53.65625, 1));
        list.uniformMat4(tileShader.uModelMatrix, &testTileModel);

        static i32 slot = 0;
        list.uniformInt(tileShader.uDiffuse, &slot);

        tileVertexMutex.lock();
        i32 tileMeshId = 0;
        for(i32 y = 0; y < 64; ++y) {
            for(i32 x = 0; x < 64; ++x) {
                i32 id = y*64 + x;
                const WldxEntry& we = sectorEntries[id];
                u32 color = 0xff000000;

                switch(dbgViewMode) {
                    case MODE_NORMAL:
                        color = 0xffffffff;
                        break;

                    case MODE_STATIC:
                        color = 0xffffffff;
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

                    case MODE_POS_INFO: {
                        if(we.floorId) {
                            color = 0xffff00ff;
                        }
                        break;
                    }
                }

                makeTileMesh(&tileVertexData[6 * (tileMeshId++)], baseTileIds[id] % 18, x, y, color);
            }
        }

        tileMeshId = 0;
        for(i32 i = 0; i < floorCount; ++i) {
            i32 posIndx = floorPosIndex[i];
            i32 x = posIndx & 63;
            i32 y = posIndx / 64;
            makeTileMesh(&tileFloorVertexData[6 * (tileMeshId++)], floorTileIds[i] % 18, x, y, 0xffffffff);
        }
        tileVertexMutex.unlock();

        list.mutexLock(&tileVertexMutex);
        list.arrayBufferSubData(&tileShader.vbo, 0, tileVertexData, sizeof(tileVertexData));
        list.arrayBufferSubData(&tileShader.vbo, sizeof(tileVertexData), tileFloorVertexData,
                                sizeof(tileVertexData[0]) * tileMeshId * 6);
        list.mutexUnlock(&tileVertexMutex);
        renderer_pushCommandList(list);


        list.bindVertexArray(&tileShader.vao);
        list.setTransparencyEnabled(false);

        // DRAW BASE TILE MESH
        // TODO: pack same texture calls or bind more textures
        for(i32 i = 0; i < dbgTileDrawCount; ++i) {
            list.textureSlot(gpuBaseTex[i], 0);
            list.drawTriangles(i * 6, 6);

            if(i % 200 == 0) {
                renderer_pushCommandList(list);
            }
        }

        renderer_pushCommandList(list);
        list.setTransparencyEnabled(true);

        // DRAW FLOOR MESH
        for(i32 i = 0; i < floorCount; ++i) {
            list.textureSlot(gpuFloorTex[i], 0);
            list.drawTriangles(MAX_TILE_COUNT * 6 + i * 6, 6);

            if(i % 200 == 0) {
                renderer_pushCommandList(list);
            }
        }

        renderer_pushCommandList(list);
    }

    void drawFloorTest()
    {
#if 0
        constexpr i32 MAX_SECTOR_TEXTURE_COUNT = 3000;
        constexpr i32 MAX_TILE_COUNT_HALF = MAX_TILE_COUNT/2;
        i32 diskSectorTexs[MAX_SECTOR_TEXTURE_COUNT] = {};
        GLuint* gpuFloorTex[MAX_SECTOR_TEXTURE_COUNT];
        i32 floorTexCount = 0;
        GLuint* tileGpuTexId[MAX_TILE_COUNT];
        i32 floorTileIds[MAX_TILE_COUNT];
        i32 floorTexIds[MAX_TILE_COUNT];
        u32 floorColor[MAX_TILE_COUNT];

        u16* tileTexIds = resource_getTileTextureIds18();
        FloorEntry* floors = resource_getFloors();
        const i32 floorCount = resource_getFloorCount();

        i32 fid = dbgFloorOffset;
        for(i32 i = 0; i < MAX_TILE_COUNT_HALF; ++i) {
            assert(fid < floorCount);

            FloorEntry& fe = floors[fid];
            i32 tileId1 = fe.pakTileIds & 0x1FFFF;
            i32 tileId2 = fe.pakTileIds >> 17;
            i32 texId1 = tileTexIds[tileId1/18];
            i32 texId2 = tileTexIds[tileId2/18];
            floorTileIds[i*2] = tileId1;
            floorTileIds[i*2+1] = tileId2;
            floorTexIds[i*2] = texId1;
            floorTexIds[i*2+1] = texId2;
            floorColor[i*2] = 0xffffffff;
            floorColor[i*2+1] = tileId2 ? 0xffffffff : 0xff000000;

            bool found1 = false;
            bool found2 = false;
            for(i32 j = 0; j < floorTexCount && (!found2 || !found2); ++j) {
                if(diskSectorTexs[j] == texId1) {
                    found1 = true;
                }
                if(diskSectorTexs[j] == texId2) {
                    found2 = true;
                }
            }

            if(!found1) {
                diskSectorTexs[floorTexCount++] = texId1;
                assert(floorTexCount <= MAX_SECTOR_TEXTURE_COUNT);
            }
            if(!found2) {
                diskSectorTexs[floorTexCount++] = texId2;
                assert(floorTexCount <= MAX_SECTOR_TEXTURE_COUNT);
            }

            if(fe.nextFloorId) {
                fid = fe.nextFloorId;
            }
            else {
                fid++;
            }
        }

        resource_requestGpuTextures(diskSectorTexs, gpuFloorTex, floorTexCount);

        // assign gpu textures to tiles
        for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
            i32 texId = floorTexIds[i];

            for(i32 j = 0; j < floorTexCount; ++j) {
                if(diskSectorTexs[j] == texId) {
                    tileGpuTexId[i] = gpuFloorTex[j];
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

        list.mutexLock(&viewMutex);
        list.uniformMat4(tileShader.uViewMatrix, &viewIso);
        list.mutexUnlock(&viewMutex);

        testTileModel = mat4Scale(vec3f(53.65625, 53.65625, 1));
        list.uniformMat4(tileShader.uModelMatrix, &testTileModel);

        static i32 slot = 0;
        list.uniformInt(tileShader.uDiffuse, &slot);

        const i32 over = dbgFloorOver;
        tileVertexMutex.lock();
        i32 tileMeshId = 0;
        for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
            //i32 fid = i*2 + over;
            i32 fid = i;
            i32 tileId = floorTileIds[fid];
            i32 x = i & 63;
            i32 y = i / 64;
            makeTileMesh(&tileVertexData[6 * (tileMeshId++)], tileId % 18, x, y, floorColor[fid]);
        }
        tileVertexMutex.unlock();

        list.mutexLock(&tileVertexMutex);
        list.arrayBufferSubData(&tileShader.vertexBuffer, 0, tileVertexData, sizeof(tileVertexData));
        list.mutexUnlock(&tileVertexMutex);
        renderer_pushCommandList(list);

        list.bindVertexArray(&tileShader.vao);

        // TODO: pack same texture calls or bind more textures
        for(i32 i = 0; i < MAX_TILE_COUNT; ++i) {
            list.textureSlot(tileGpuTexId[i], 0);
            list.drawTriangles(i * 6, 6);

            if(i % 200 == 0) {
                renderer_pushCommandList(list);
            }
        }

        renderer_pushCommandList(list);
#endif
    }

    void drawTestMixed()
    {
        constexpr i32 TEX_MAXCOUNT = 10;
        constexpr i32 VERTICES_MAXCOUNT = 6 * 256;
        static GLuint* gpuMixedTex[TEX_MAXCOUNT];
        static i32 mixedTexIds[TEX_MAXCOUNT];
        static TileVertex mixedMesh[VERTICES_MAXCOUNT];
        i32 mixedTexCount = 0;
        i32 mixedVertCount = 0;

        const PakMixedDesc* descs = resource_getMixedDescs();
        const PakMixedData* mixed = resource_getMixedData();
        const i32 descCount = resource_getMixedDescsCount();

        assert(dbgMixedId < descCount);
        const i32 mixedCount = descs[dbgMixedId].count;
        const i32 mixedStartId = descs[dbgMixedId].mixedDataId;

        if(mixedCount == 0) {
            return;
        }

        tileVertexMutex.lock();
        for(i32 i = 0; i < mixedCount; ++i) {
            const PakMixedData& md = mixed[i + mixedStartId];
            mixedTexIds[mixedTexCount++] = md.textureId;

            /*meshAddQuad(mixedMesh + mixedVertCount, md.x, md.y, md.width, md.height, md.uvX1, md.uvY1,
                        md.uvX2, md.uvY2, 0xffffffff);*/
            i32 w = md.width - md.x;
            i32 h = md.height - md.y;
            meshAddQuad(mixedMesh + mixedVertCount, md.x, md.y, w, h, md.uvX1, md.uvY1,
                        md.uvX2, md.uvY2, 0xffffffff);
            mixedVertCount += 6;
        }
        tileVertexMutex.unlock();

        resource_requestGpuTextures(mixedTexIds, gpuMixedTex, mixedTexCount);

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

        list.mutexLock(&viewMutex);
        list.uniformMat4(tileShader.uViewMatrix, &viewIso);
        list.mutexUnlock(&viewMutex);

        static mat4 mixedModel = mat4Scale(vec3f(5, 5, 1));
        list.uniformMat4(tileShader.uModelMatrix, &mixedModel);

        static i32 slot = 0;
        list.uniformInt(tileShader.uDiffuse, &slot);

        list.mutexLock(&tileVertexMutex);
        list.arrayBufferSubData(&tileShader.vbo, 0, mixedMesh, sizeof(mixedMesh[0]) * mixedVertCount);
        list.mutexUnlock(&tileVertexMutex);
        renderer_pushCommandList(list);


        list.bindVertexArray(&tileShader.vao);
        list.setTransparencyEnabled(true);

        i32 quadCount = mixedVertCount / 6;
        for(i32 i = 0; i < quadCount; ++i) {
            list.textureSlot(gpuMixedTex[i], 0);
            list.drawTriangles(i * 6, 6);

            if(i % 200 == 0) {
                renderer_pushCommandList(list);
            }
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
    ImGui::SetNextWindowPos(ImVec2(5,5));
    ImGui::Begin("Video info", nullptr,
                 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
                 ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Current/Total %dmb/%dmb", (availMemory-currentAvailMem)/1024, availMemory/1024);
    ImGui::ProgressBar(1.0 - (currentAvailMem / (f64)availMemory));
    ImGui::Separator();
    ImGui::Text("Renderer frametime: %.5fms", renderer_getFrameTime() * 1000.0);
    ImGui::Text("Game     frametime: %.5fms", pGame->frameTime * 1000.0);

    ImGui::End();
}

unsigned long thread_game(void*)
{
    LOG("thread_game started [%x]", threadGetId());

    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    initTileUVs();

    renderer_waitForInit();
    client.dbguiWaitForInit();

    if(!resource_init()) {
        client.clientRunning = false;
        return 1;
    }

    Game game;
    pGame = &game;

    client.addInputCallback(receiveGameInput, &game);
    game.loadShaders();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    LOG_SUCC("Game> initializated");

    while(client.clientRunning) {
        auto t0 = timeNow();
        game.processInput();
        resource_newFrame();

        game.requestTexBrowserTextures();
        game.requestFloorTextures();

        // NOTE: dont push render command inside UI code
#ifdef CONF_ENABLE_UI
        client.dbguiNewFrameBegin();
        if(client.imguiSetup) {
            game.ui_textureBrowser();
            //game.ui_tileTest();
            //game.ui_floorTest();
            game.ui_mixedViewer();
            ui_videoInfo();

            //ImGui::ShowTestWindow();
        }
        client.dbguiNewFrameEnd();
#endif

        game.loadSectorIfNeeded();

        CommandList list;
        list.clear();
        renderer_pushCommandList(list);

        //game.drawTestTiles();
        //game.drawFloorTest();
        game.drawTestMixed();

        list.queryVramInfo(&dedicated, &availMemory, &currentAvailMem, &evictionCount, &evictedMem);
        list.endFrame();
        renderer_pushCommandList(list);
        game.frameTime = timeDurSince(t0);
    }

    // TODO: wait for deinit
    LOG("Game> cleaning up...");
    game.deinit();

    return 0;
}
