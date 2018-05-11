#include "rs_game.h"
#include "rs_logger.h"
#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_file.h"
#include "rs_math.h"
#include "imgui.h"
#include "stb_image.h"
#include "rs_resources.h"
#include "rs_dbg_draw.h"



// interesting sectors:
/*
 * 130
 * 246
 * 250
 * 299
 * 690
 * 1222
 * 1360
 * 1750
 * 1752
 * 2123
 * 2529
 * 3810 <<<
 * 4859
 * 5351
*/

// interesting texture page: 27 (minimap)

#define PAGE_TEXTURES_COUNT 160
#define VIEW_X_ANGLE (1.04719755119659774615)
#define TILE_WIDTH 64

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

void makeTileMesh(TileVertex* mesh, i32 localTileId, f32 x, f32 y, f32 z, u32 color, i32 alphaMaskTileId = 0)
{
    assert(localTileId >= 0 && localTileId < 18);

    if(alphaMaskTileId) {
        mesh[0] = TileVertex(0.0 + x, 0.0 + y, z,
                tileUV[localTileId][1].x, tileUV[localTileId][1].y,
                tileUV[alphaMaskTileId][1].x, tileUV[alphaMaskTileId][1].y,
                color);
        mesh[1] = TileVertex(1.0 + x, 0.0 + y, z,
                tileUV[localTileId][2].x, tileUV[localTileId][2].y,
                tileUV[alphaMaskTileId][2].x, tileUV[alphaMaskTileId][2].y,
                color);
        mesh[2] = TileVertex(1.0 + x, 1.0 + y, z,
                tileUV[localTileId][3].x, tileUV[localTileId][3].y,
                tileUV[alphaMaskTileId][3].x, tileUV[alphaMaskTileId][3].y,
                color);
        mesh[3] = TileVertex(0.0 + x, 0.0 + y, z,
                tileUV[localTileId][1].x, tileUV[localTileId][1].y,
                tileUV[alphaMaskTileId][1].x, tileUV[alphaMaskTileId][1].y,
                color);
        mesh[4] = TileVertex(1.0 + x, 1.0 + y, z,
                tileUV[localTileId][3].x, tileUV[localTileId][3].y,
                tileUV[alphaMaskTileId][3].x, tileUV[alphaMaskTileId][3].y,
                color);
        mesh[5] = TileVertex(0.0 + x, 1.0 + y, z,
                tileUV[localTileId][0].x, tileUV[localTileId][0].y,
                tileUV[alphaMaskTileId][0].x, tileUV[alphaMaskTileId][0].y,
                color);
    }
    else {
        mesh[0] = TileVertex(0.0 + x, 0.0 + y, z,
                tileUV[localTileId][1].x, tileUV[localTileId][1].y,
                -1, -1,
                color);
        mesh[1] = TileVertex(1.0 + x, 0.0 + y, z,
                tileUV[localTileId][2].x, tileUV[localTileId][2].y,
                -1, -1,
                color);
        mesh[2] = TileVertex(1.0 + x, 1.0 + y, z,
                tileUV[localTileId][3].x, tileUV[localTileId][3].y,
                -1, -1,
                color);
        mesh[3] = TileVertex(0.0 + x, 0.0 + y, z,
                tileUV[localTileId][1].x, tileUV[localTileId][1].y,
                -1, -1,
                color);
        mesh[4] = TileVertex(1.0 + x, 1.0 + y, z,
                tileUV[localTileId][3].x, tileUV[localTileId][3].y,
                -1, -1,
                color);
        mesh[5] = TileVertex(0.0 + x, 1.0 + y, z,
                tileUV[localTileId][0].x, tileUV[localTileId][0].y,
                -1, -1,
                color);
    }
}

void meshAddQuad(TileVertex* mesh, f32 x, f32 y, f32 z, f32 width, f32 height, f32 uvX1, f32 uvY1,
                 f32 uvX2, f32 uvY2, u32 color)
{
    mesh[0] = TileVertex(x        , y         , z, uvX1, uvY1, -1, -1, color);
    mesh[1] = TileVertex(x + width, y         , z, uvX2, uvY1, -1, -1, color);
    mesh[2] = TileVertex(x + width, y + height, z, uvX2, uvY2, -1, -1, color);
    mesh[3] = TileVertex(x        , y         , z, uvX1, uvY1, -1, -1, color);
    mesh[4] = TileVertex(x + width, y + height, z, uvX2, uvY2, -1, -1, color);
    mesh[5] = TileVertex(x        , y + height, z, uvX1, uvY2, -1, -1, color);
}

#define MAX_TILE_COUNT 4096
#define MAX_MIXED_QUAD 7168

static TileVertex tileVertexData[6 * MAX_TILE_COUNT];
static TileVertex tileFloorVertexData[6 * MAX_TILE_COUNT];
static TileVertex mixedQuadMesh[6 * MAX_MIXED_QUAD];

struct Game
{
RendererFrameData frameData;
i32 winWidth;
i32 winHeight;
f64 frameTime = 0;
i32 texBrowser_texIds[PAGE_TEXTURES_COUNT];
GLuint* texBrowser_texGpu[PAGE_TEXTURES_COUNT];
i32 pageId = 0;
i32 testTileLocalId = 0;
MutexSpin tileVertexMutex;
MutexSpin tileFloorVertexMutex;
MutexSpin mixedQuadMeshMutex;

GLuint vboBaseTileMesh;
GLuint vboFloorTileMesh;

bool viewIsIso = true;
mat4 matProjOrtho;
mat4 matIsoRotation;
mat4 matViewOrtho;
mat4 matViewIso;
mat4 testTileModel;
f32 viewX = 0;
f32 viewY = 0;
f32 viewZoom = 1.0f;
i32 viewZMul = -1;
MutexSpin viewMutex;

bool showUi = true;
i32 dbgTileDrawCount = MAX_TILE_COUNT;
i32 dbgSectorId = 2529;
i32 loadedSectorId = -1;
SectorxData* sectorData = nullptr;
SectorInfo sectorInfo;

i32 dbgFloorOffset = 0;
bool dbgFloorOver = 0;
i32 dbgOverlayFloor = 1;

GLuint* gpuFloorTexs[MAX_TILE_COUNT];
i32 floorTexCount = 0;

i32 dbgMixedId = 0;
bool dbgShowMixed = true;
f32 dbgMixedOffX = 0.0;
f32 dbgMixedOffY = 0.0;
bool dbgMixedUseBaseOffset = false;
bool dbgMixedUseStaticOffset = true;
i32 dbgMixedObjMax = 4096;
f32 dbgTileWidth = 67.9f;

i32 dbgHoveredTileId = 0;
i32 dbgSelectedTileId = -1;

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
    "SMTH_TYPE"
};
i32 dbgViewMode = MODE_NORMAL;

struct {
    u8 mouseLeft = 0;
    u8 mouseRight = 0;

    i32 mouseX = 0;
    i32 mouseY = 0;
    i32 mouseRelX = 0;
    i32 mouseRelY = 0;
    i32 mouseWheelRelY = 0;
} input;

inline vec3f posOrthoToIso(vec3f v)
{
    return vec3fMulMat4(v, matIsoRotation);
}

inline vec3f sacred_worldToScreen(vec3f v)
{
    return vec3f(v.x * 0.89442718 + v.y * -0.89442718,
                 v.x * 0.44721359 + v.y * 0.44721359,
                 v.z);
}

inline vec3f sacred_screenToWorld(vec3f v)
{
    return vec3f((v.x * 0.44721359 - v.y * -0.89442718) * 1.25,
                 (v.y * 0.89442718 - v.x * 0.44721359) * 1.25,
                 v.z);
}

void init()
{
#if 0
    CommandList list;
    list.arrayBufferData(&tileShader.vbo, 0, sizeof(tileVertexData) +
                         sizeof(tileFloorVertexData) +
                         sizeof(mixedQuadMesh),
                         GL_STATIC_DRAW);
    renderer_pushCommandList(list);
#endif

    Window& client = *get_clientWindow();
    winWidth = client.width;
    winHeight = client.height;
    matProjOrtho = mat4Orthographic(0, winWidth, winHeight, 0, -10000.f, 10000.f);
    matIsoRotation = mat4RotateAxisX(VIEW_X_ANGLE);
    matIsoRotation = mat4Mul(matIsoRotation, mat4RotateAxisZ(-RS_HALFPI/2));
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
    ImGui::Text("Game     frametime: %.5fms", frameTime * 1000.0);

    ImGui::End();
}

void ui_all()
{
    if(!showUi) return;

    ui_textureBrowser();
    ui_tileTest();
    //game.ui_floorTest();
    ui_mixedViewer();
    ui_videoInfo();

    ui_tileInspector();

    //ImGui::ShowTestWindow();
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

    ImGui::Checkbox("Isometric view", &viewIsIso);
    ImGui::Combo("viewMode", &dbgViewMode, viewModeCombo, MODE_COUNT);

    //ImGui::SliderInt("dbgTileDrawCount", &dbgTileDrawCount, 1, 4096);


    ImGui::SliderInt("Sector ID", &dbgSectorId, 1, 6049);
    ImGui::InputInt("##sector_id_input", &dbgSectorId);

    ImGui::SliderFloat("TILE_WIDTH", &dbgTileWidth, 1, 200, "%.1f");
    ImGui::SliderFloat("mixOffX", &dbgMixedOffX, -200, 200, "%.1f");
    ImGui::SliderFloat("mixOffY", &dbgMixedOffY, -200, 200, "%.1f");
    ImGui::Checkbox("Mixed use base offset", &dbgMixedUseBaseOffset);
    ImGui::Checkbox("Mixed use static offset", &dbgMixedUseStaticOffset);
    ImGui::SliderInt("Mixed max objects drawn", &dbgMixedObjMax, 0, 4096);

    dbgSectorId = clamp(dbgSectorId, 1, 6049);

    ImGui::SliderInt("Overlay floors", &dbgOverlayFloor, 0, 2);


    if(sectorData) {
        ImGui::Text("#%d (%s)", dbgSectorId, sectorData->name);
        ImGui::Text("posX1: %d posX2: %d", sectorInfo.posX1, sectorInfo.posX2);
        ImGui::Text("posY1: %d posY2: %d", sectorInfo.posY1, sectorInfo.posY2);

        vec3f sectorPos(sectorInfo.posX1, sectorInfo.posY1, 0);

        sectorPos.x = sectorInfo.posX1 * 0.89442718 + sectorInfo.posY1 * -0.89442718;
        sectorPos.y = sectorInfo.posX1 * 0.44721359 + sectorInfo.posY1 * 0.44721359;

        ImGui::Text("x: %d y: %d", (i32)sectorPos.x, (i32)sectorPos.y);

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

        ImGui::Checkbox("Show mixed", &dbgShowMixed);

        if(ImGui::Button("Dump data to file")) {
            char path[256];
            sprintf(path, "sector_data.%d", dbgSectorId);
            fileWriteBuffer(path, (const char*)sectorData, sectorInfo.uncompressedSize);
        }
    }

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
    PakMixedDesc* desc = resource_getMixedDescs();

    ImGui::Begin("Mixed viewer");

    ImGui::SliderInt("id", &dbgMixedId, 0, descCount);
    ImGui::InputInt("##id_input", &dbgMixedId);

    if(ImGui::Button("Skip 0s")) {
        for(; dbgMixedId < descCount; dbgMixedId++) {
            if(desc[dbgMixedId].count > 0) {
                break;
            }
        }
    }

    dbgMixedId = clamp(dbgMixedId, 0, descCount);
    const PakMixedDesc& d = desc[dbgMixedId];

    ImGui::Text("count    : %d", d.count);
    ImGui::Text("width    : %d", d.width);
    ImGui::Text("height   : %d", d.height);
    ImGui::Text("offx     : %d", d.offX);
    ImGui::Text("offy     : %d", d.offY);

    ImGui::End();
}

void ui_tileInspector()
{
    if(dbgSelectedTileId == -1 || loadedSectorId != dbgSectorId) return;
    const WldxEntry& tile = sectorData->data[dbgSelectedTileId];

    ImGui::Begin("Tile inspector");

    ImGui::Text("Tile %d", dbgSelectedTileId);

    ImGui::Separator();

    ImGui::Text("texture tileId: %d", tile.tileId);

    ImGui::Text("staticId: %d", tile.staticId);
    if(tile.staticId && ImGui::CollapsingHeader("Static")) {
        const ImVec4 staticCol(0, 1, 1, 1);
        assert(tile.staticId < resource_getStaticCount());
        const PakStatic& s = resource_getStatic()[tile.staticId];

        ImGui::Text("itemTypeId     : %d", s.itemTypeId);
        if(s.itemTypeId && ImGui::CollapsingHeader("ItemType")) {
            const ImVec4 itemTypeCol(1, 1, 0, 1);
            assert(s.itemTypeId < resource_getItemTypesCount());
            PakItemType& itemType = resource_getItemTypes()[s.itemTypeId];

            constexpr auto unkElementGet = [](void* data, i32 id, const char** outStr) -> bool {
                static char outBuff[32];
                sprintf(outBuff, "%d", ((u8*)data)[id]);
                *outStr = outBuff;
                return true;
            };

            ImGui::TextColored(itemTypeCol, "flags          : %x", itemType.flags);
            ImGui::TextColored(itemTypeCol, "int_1          : %d", itemType.int_1);
            ImGui::TextColored(itemTypeCol, "itemTextureId  : %d", itemType.itemTextureId);
            ImGui::TextColored(itemTypeCol, "field_C        : %d", itemType.field_C);
            ImGui::TextColored(itemTypeCol, "mixedId        : %d", itemType.mixedId);
            ImGui::TextColored(itemTypeCol, "field_14       : %d", itemType.field_14);
            ImGui::TextColored(itemTypeCol, "spawnInfoId    : %d", itemType.spawnInfoId);
            ImGui::TextColored(itemTypeCol, "field_1C       : %d", itemType.field_1C);
            ImGui::TextColored(itemTypeCol, "field_20       : %d", itemType.field_20);
            ImGui::TextColored(itemTypeCol, "soundProfileId : %d", itemType.soundProfileId);

            static i32 unk_id = 0;
            ImGui::ListBox("unk", &unk_id, unkElementGet, itemType.unk, IM_ARRAYSIZE(itemType.unk),
                           IM_ARRAYSIZE(itemType.unk));

            ImGui::TextColored(itemTypeCol, "unk3           : %d", itemType.unk3);
            ImGui::TextColored(itemTypeCol, "category       : %d", itemType.category);

            static i32 unk4_id = 0;
            ImGui::ListBox("unk4", &unk4_id, unkElementGet, itemType.unk4,
                           IM_ARRAYSIZE(itemType.unk4), IM_ARRAYSIZE(itemType.unk4));

            ImGui::TextColored(itemTypeCol, "name           : %s", itemType.nameStr);
            ImGui::TextColored(itemTypeCol, "field_57       : %d", itemType.field_57);
            ImGui::TextColored(itemTypeCol, "field_59       : %d", itemType.field_59);
            ImGui::TextColored(itemTypeCol, "someVectorId   : %d", itemType.someVectorId);
            ImGui::TextColored(itemTypeCol, "marginX        : %d", itemType.marginX);
            ImGui::TextColored(itemTypeCol, "marginY        : %d", itemType.marginY);

            static i32 unk2_id = 0;
            ImGui::ListBox("unk2", &unk2_id, unkElementGet, itemType.unk2, IM_ARRAYSIZE(itemType.unk2), 5);

            ImGui::Separator();
        }

        ImGui::TextColored(staticCol, "field_8        : %d", s.field_8);
        ImGui::TextColored(staticCol, "field_C        : %d", s.field_C);
        ImGui::TextColored(staticCol, "worldX         : %d", s.worldX);
        ImGui::TextColored(staticCol, "worldY         : %d", s.worldY);

        vec3f posIso = sacred_screenToWorld(vec3f(s.worldX, s.worldY, 0));
        ImGui::TextColored(staticCol, "isoX           : %g", posIso.x);
        ImGui::TextColored(staticCol, "isoY           : %g", posIso.y);

        ImGui::TextColored(staticCol, "unk_0          : %d", s.unk_0);
        ImGui::TextColored(staticCol, "parentId       : %d", s.parentId);
        ImGui::TextColored(staticCol, "anotherParentId: %d", s.anotherParentId);
        ImGui::TextColored(staticCol, "nextStaticId   : %d", s.nextStaticId);
        ImGui::TextColored(staticCol, "parentOffsetTx : %d", s.parentOffsetTx);
        ImGui::TextColored(staticCol, "parentOffsetTy : %d", s.parentOffsetTy);
        ImGui::TextColored(staticCol, "triggerId      : %d", s.triggerId);

        ImGui::Separator();

        ImGui::TextColored(staticCol, "field_2B      : %d", s.field_2B);
        ImGui::TextColored(staticCol, "field_2C      : %d", s.field_2C);
        ImGui::TextColored(staticCol, "layer         : %d", s.layer);
        ImGui::TextColored(staticCol, "smthX         : %d", s.smthX);
        ImGui::TextColored(staticCol, "smthY         : %d", s.smthY);
        ImGui::TextColored(staticCol, "smthZ         : %d", s.smthZ);
        ImGui::TextColored(staticCol, "field_31      : %d", s.field_31);
        ImGui::TextColored(staticCol, "field_32      : %d", s.field_32);
        ImGui::TextColored(staticCol, "field_33      : %d", s.field_33);
        ImGui::TextColored(staticCol, "unk_1         : %d", s.unk_1);
        ImGui::TextColored(staticCol, "field_35      : %d", s.field_35);

        ImGui::Separator();
    }

    ImGui::Text("floorId: %d", tile.floorId);
    ImGui::Text("rest: %llx", *(u64*)tile.rest);
    ImGui::Text("smth: %d %d %d %d", tile.smthX, tile.smthY, tile.smthZ, tile.smthW);
    ImGui::Text("offset: %d %d", tile.offsetX, tile.offsetY);
    ImGui::Text("someTypeId: %d", tile.someTypeId);

    ImGui::End();
}

void drawSector()
{
#if 0
    static GLuint* gpuBaseTex[MAX_TILE_COUNT];
    static i32 baseTileIds[MAX_TILE_COUNT];
    static i32 baseTileTexIds[MAX_TILE_COUNT];

    static GLuint* gpuFloorTex[MAX_TILE_COUNT];
    static GLuint* gpuFloorTexAlphaMask[MAX_TILE_COUNT];
    static i32 floorTileIds[MAX_TILE_COUNT];
    static i32 floorAlphaMaskTileIds[MAX_TILE_COUNT];
    static i32 floorTileTexIds[MAX_TILE_COUNT];
    static i32 floorAlphaMaskTexIds[MAX_TILE_COUNT];
    static i32 floorPosIndex[MAX_TILE_COUNT];
    i32 floorCount = 0;

    static GLuint* gpuMixedQuadTex[MAX_MIXED_QUAD];
    static i32 mixedQuadTexId[MAX_MIXED_QUAD];
    i32 mixedQuadCount = 0;


    const WldxEntry* sectorEntries = sectorData->data;
    const u16* tileTexIds = resource_getTileTextureIds18();
    const i32 tileCount = resource_getTileCount18() * 18;

    const FloorEntry* floors = resource_getFloors();
    const i32 floorMaxCount = resource_getFloorCount();

    const PakStatic* statics = resource_getStatic();
    const PakMixedDesc* mixedDescs = resource_getMixedDescs();
    const PakMixedData* mixed = resource_getMixedData();
    const PakItemType* itemTypes = resource_getItemTypes();
    const i32 mixedDescCount = resource_getMixedDescsCount();
    const i32 staticCount = resource_getStaticCount();
    const i32 itemTypeCount = resource_getItemTypesCount();

    mixedQuadMeshMutex.lock();

    i32 mixedObjCount = 0;
    i32 worldOriginX, worldOriginY;
    resource_getWorldOrigin(&worldOriginX, &worldOriginY);

    const i32 sectorX = sectorInfo.posX1;
    const i32 sectorY = sectorInfo.posY1;
    const vec3f sectorSceen = sacred_worldToScreen(vec3f(sectorX, sectorY, 0));

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
            i32 tileId = floors[floorId].pakTileIds & 0x1FFFF;
            i32 tileIdAlphaMask = floors[floorId].pakTileIds >> 17;

            /*LOG_DBG("%d %x tileId1=%d tileId2=%d", i,
                    floors[we.floorId].pakTileIds,
                    floors[we.floorId].pakTileIds & 0x1FFFF,
                    floors[we.floorId].pakTileIds >> 17);*/

            if(tileId > 0) {
                assert(floorCount < MAX_TILE_COUNT);
                i32 fid = floorCount++;
                floorTileIds[fid] = tileId;
                floorTileTexIds[fid] = tileTexIds[tileId/18];
                floorAlphaMaskTileIds[fid] = tileIdAlphaMask;
                floorAlphaMaskTexIds[fid] = tileTexIds[tileIdAlphaMask/18];
                floorPosIndex[fid] = i;
            }
            floorId = floors[floorId].nextFloorId;
        }

        if(we.staticId) {
            assert(we.staticId < staticCount);
            const PakStatic& sta = statics[we.staticId];
            i32 itemTypeId = sta.itemTypeId;

            if(itemTypeId) {
                assert(itemTypeId < itemTypeCount);
                i32 mixedId = itemTypes[itemTypeId].mixedId;

                if(mixedId && dbgShowMixed && mixedObjCount < dbgMixedObjMax) {
                    mixedObjCount++;
                    assert(mixedId < mixedDescCount);
                    const PakMixedDesc& desc = mixedDescs[mixedId];
                    const i32 mcount = desc.count;
                    const i32 mixedStartId = desc.mixedDataId;

                    f32 orgnX = sta.worldX - sectorSceen.x;
                    f32 orgnY = sta.worldY - sectorSceen.y;
                    mat4 inv = mat4Inv(matIsoRotation);
                    vec3f orgnPosIso = vec3fMulMat4(vec3f(orgnX, orgnY, 0), inv);

                    if(viewIsIso) {
                        orgnPosIso = posOrthoToIso(orgnPosIso);
                    }

                    for(i32 m = 0; m < mcount; ++m) {
                        const PakMixedData& md = mixed[m + mixedStartId];
                        assert(mixedQuadCount < MAX_MIXED_QUAD);
                        i32 mid = mixedQuadCount++;
                        mixedQuadTexId[mid] = md.textureId;

                        f32 x = orgnPosIso.x + md.x;
                        f32 y = orgnPosIso.y + md.y;
                        i32 w = md.width - md.x;
                        i32 h = md.height - md.y;
                        meshAddQuad(mixedQuadMesh + mid * 6, x, y, 0.0, w, h, md.uvX1, md.uvY1,
                                    md.uvX2, md.uvY2, 0xffffffff);
                    }
                }
            }
        }
    }

    mixedQuadMeshMutex.unlock();

    resource_requestGpuTextures(baseTileTexIds, gpuBaseTex, MAX_TILE_COUNT);
    resource_requestGpuTextures(floorTileTexIds, gpuFloorTex, floorCount);
    resource_requestGpuTextures(floorAlphaMaskTexIds, gpuFloorTexAlphaMask, floorCount);
    resource_requestGpuTextures(mixedQuadTexId, gpuMixedQuadTex, mixedQuadCount);

    CommandList list;
    list.useProgram(&tileShader.program);

    static mat4 viewOrtho, viewIso;
    viewOrtho = matViewOrtho;
    viewIso = matViewIso;

    list.uniformMat4(tileShader.uProjMatrix, &matProjOrtho);
    if(viewIsIso) {
        list.uniformMat4(tileShader.uViewMatrix, &viewIso);
    }
    else {
        list.uniformMat4(tileShader.uViewMatrix, &viewOrtho);
    }

    testTileModel = mat4Scale(vec3f(dbgTileWidth, dbgTileWidth, 1));
    list.uniformMat4(tileShader.uModelMatrix, &testTileModel);

    static i32 diffSlot = 0;
    static i32 alphaSlot = 1;
    list.uniformInt(tileShader.uDiffuse, &diffSlot);
    list.uniformInt(tileShader.uAlphaMask, &alphaSlot);

    tileVertexMutex.lock();
    i32 tileMeshId = 0;
    for(i32 y = 0; y < 64; ++y) {
        for(i32 x = 0; x < 64; ++x) {
            i32 id = y*64 + x;
            const WldxEntry& we = sectorEntries[id];
            u32 color = 0xff000000;

            if(id == dbgSelectedTileId) {
                color = 0xffff0000;
            }
            else if(id == dbgHoveredTileId) {
                color = 0xffff8f8f;
            }
            else {
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
                        color = 0xffffffff;
                        if(we.entityId) {
                            color = 0xffff0000;
                        }
                        break;

                    case MODE_SMTH_TYPE:
                        color = 0xffffffff;
                        if(we.someTypeId) {
                            color = 0xff000000 | (0x00000001 * (we.someTypeId*(255/15)));
                        }
                        break;

                    case MODE_SMTH_POS:
                        color = 0xff000000 | (we.smthZ << 16) | (we.smthY << 8) | (we.smthX);
                        break;
                }
            }

            if(color != 0xffffffff) {
                dbgDrawSolidSquare(vec3f(x*dbgTileWidth, y*dbgTileWidth, 0),
                                   vec3f(dbgTileWidth, dbgTileWidth, 0), color,
                                   DbgCoordSpace::WORLD);
            }

            makeTileMesh(&tileVertexData[6 * (tileMeshId++)], baseTileIds[id] % 18, x, y, 0.0,
                    0xffffffff);
        }
    }

    tileMeshId = 0;
    for(i32 i = 0; i < floorCount; ++i) {
        i32 posIndx = floorPosIndex[i];
        i32 x = posIndx & 63;
        i32 y = posIndx / 64;
        makeTileMesh(&tileFloorVertexData[6 * i], floorTileIds[i] % 18, x, y, 0.0, 0xffffffff,
                    floorAlphaMaskTileIds[i] % 18);
    }
    tileVertexMutex.unlock();

    list.mutexLock(&tileVertexMutex);
    i32 offset = 0;
    list.arrayBufferSubData(&tileShader.vbo, offset, tileVertexData, sizeof(tileVertexData));
    offset += sizeof(tileVertexData);

    list.arrayBufferSubData(&tileShader.vbo, offset, tileFloorVertexData,
                            sizeof(tileVertexData[0]) * floorCount * 6);
    offset += sizeof(tileFloorVertexData[0]) * floorCount * 6;
    list.mutexUnlock(&tileVertexMutex);

    list.mutexLock(&mixedQuadMeshMutex);
    list.arrayBufferSubData(&tileShader.vbo, offset, mixedQuadMesh,
                            sizeof(mixedQuadMesh[0]) * mixedQuadCount * 6);
    list.mutexUnlock(&mixedQuadMeshMutex);

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
    i32 vertOffset = MAX_TILE_COUNT * 6;
    for(i32 i = 0; i < floorCount; ++i) {
        list.textureSlot(gpuFloorTex[i], diffSlot);
        list.textureSlot(gpuFloorTexAlphaMask[i], alphaSlot);
        list.drawTriangles(vertOffset + i * 6, 6);

        if(i % 200 == 0) {
            renderer_pushCommandList(list);
        }
    }

    renderer_pushCommandList(list);

    static mat4 mixedModel = mat4Scale(vec3f(1, 1, 1));
    list.uniformMat4(tileShader.uViewMatrix, &viewOrtho);
    list.uniformMat4(tileShader.uModelMatrix, &mixedModel);

    // DRAW MIXED MESH
    vertOffset += floorCount * 6;
    for(i32 i = 0; i < mixedQuadCount; ++i) {
        list.textureSlot(gpuMixedQuadTex[i], 0);
        list.drawTriangles(vertOffset + i * 6, 6);

        if(i % 200 == 0) {
            renderer_pushCommandList(list);
        }
    }

    renderer_pushCommandList(list);
#endif
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
#if 0
    static GLuint* gpuMixedQuadTex[MAX_MIXED_QUAD];
    static i32 mixedQuadTexId[MAX_MIXED_QUAD];
    i32 mixedQuadCount = 0;

    const PakMixedDesc* descs = resource_getMixedDescs();
    const PakMixedData* mixed = resource_getMixedData();
    const i32 descCount = resource_getMixedDescsCount();

    assert(dbgMixedId < descCount);
    const i32 mcount = descs[dbgMixedId].count;
    const i32 mixedStartId = descs[dbgMixedId].mixedDataId;

    if(mcount == 0) {
        return;
    }

    mixedQuadMeshMutex.lock();

    for(i32 m = 0; m < mcount; ++m) {
        const PakMixedData& md = mixed[m + mixedStartId];
        assert(mixedQuadCount < MAX_MIXED_QUAD);
        i32 mid = mixedQuadCount++;
        mixedQuadTexId[mid] = md.textureId;
        f32 x = md.x;
        f32 y = md.y;
        i32 w = md.width - md.x;
        i32 h = md.height - md.y;
        meshAddQuad(mixedQuadMesh + mid * 6, x, y, 0.0, w, h, md.uvX1, md.uvY1,
                    md.uvX2, md.uvY2, 0xffffffff);
    }

    mixedQuadMeshMutex.unlock();

    resource_requestGpuTextures(mixedQuadTexId, gpuMixedQuadTex, mixedQuadCount);

    CommandList list;
    list.useProgram(&tileShader.program);

    list.uniformMat4(tileShader.uProjMatrix, &matProjOrtho);
    list.uniformMat4(tileShader.uViewMatrix, &matViewOrtho);

    //testTileModel = mat4Scale(vec3f(53.65625, 53.65625, 1));
    if(viewIsIso) {
        testTileModel = mat4Mul(matIsoRotation, mat4Scale(vec3f(dbgTileWidth, dbgTileWidth, 1)));
    }
    else {
        testTileModel = mat4Scale(vec3f(dbgTileWidth, dbgTileWidth, 1));
    }
    list.uniformMat4(tileShader.uModelMatrix, &testTileModel);

    static mat4 mixedModel = mat4Scale(vec3f(1, 1, 1));
    list.uniformMat4(tileShader.uModelMatrix, &mixedModel);

    static i32 slot = 0;
    list.uniformInt(tileShader.uDiffuse, &slot);

    list.mutexLock(&mixedQuadMeshMutex);
    list.arrayBufferSubData(&tileShader.vbo, 0, mixedQuadMesh,
                            sizeof(mixedQuadMesh[0]) * mixedQuadCount * 6);
    list.mutexUnlock(&mixedQuadMeshMutex);
    renderer_pushCommandList(list);


    list.bindVertexArray(&tileShader.vao);
    list.setTransparencyEnabled(true);

    for(i32 i = 0; i < mixedQuadCount; ++i) {
        list.textureSlot(gpuMixedQuadTex[i], 0);
        list.drawTriangles(i * 6, 6);

        if(i % 200 == 0) {
            renderer_pushCommandList(list);
        }
    }

    renderer_pushCommandList(list);
#endif
}

void updateCameraMatrices()
{
    matProjOrtho = mat4Orthographic(0, winWidth, winHeight, 0, -10000.f, 10000.f);
    matViewOrtho = mat4Mul(
                       mat4Scale(vec3f(1.0f/viewZoom, 1.0f/viewZoom, 1)),
                       mat4Translate(vec3f(-viewX, -viewY, 1))
                       );
    matViewIso = mat4Mul(matViewOrtho, matIsoRotation);
}

void render()
{
    updateCameraMatrices();

    // select tile to inspect
    const vec2f mpos(input.mouseX, input.mouseY);
    const vec3f mWorldPos = screenToWorldPos(mpos); // actual world position

    const i32 tx = mWorldPos.x / dbgTileWidth;
    const i32 ty = mWorldPos.y / dbgTileWidth;
    if(tx >= 0 && tx < 64 && ty >= 0 && ty < 64) {
        dbgHoveredTileId = ty * 64 + tx;

        if(!ImGui::GetIO().WantCaptureMouse && input.mouseLeft) {
            dbgSelectedTileId = dbgHoveredTileId;
        }
    }

    drawSector();
    //drawFloorTest();
    //drawTestMixed();

#if 1
    dbgDrawSetView(matProjOrtho, mat4Identity(), DbgCoordSpace::SCREEN);
    dbgDrawSetView(matProjOrtho, viewIsIso ? matViewIso : matViewOrtho, DbgCoordSpace::WORLD);

    // origin
    dbgDrawSolidSquare(vec3f(0,0,0), vec3f(200, 10, 1), 0xffff0000, DbgCoordSpace::WORLD);
    dbgDrawSolidSquare(vec3f(0,0,0), vec3f(10, 200, 1), 0xff00ff00, DbgCoordSpace::WORLD);
    dbgDrawSolidSquare(vec3f(0,0,0), vec3f(10, 10, 1), 0xff0000ff, DbgCoordSpace::WORLD);

    dbgDrawRender();
#endif
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
        input.mouseX = event.motion.x;
        input.mouseY = event.motion.y;
        input.mouseRelX += event.motion.xrel;
        input.mouseRelY += event.motion.yrel;
    }
    else if(event.type == SDL_MOUSEWHEEL) {
        input.mouseWheelRelY += event.wheel.y;
    }
    else if(event.type == SDL_KEYDOWN) {
        if(event.key.keysym.sym == SDLK_w) {
            showUi ^= 1;
        }
    }
}

void processInput()
{
    if(ImGui::GetIO().WantCaptureMouse) {
        return;
    }

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

// project to z=0 plane
vec3f screenToWorldPos(const vec2f screenPos)
{
    mat4 mvp;
    if(viewIsIso) {
        mvp = mat4Mul(matProjOrtho, matViewIso);
    }
    else {
        mvp = mat4Mul(matProjOrtho, matViewOrtho);
    }

    const mat4 inv = mat4Inv(mvp);
    const vec4f v = vec4fMulMat4(vec4f((screenPos.x / winWidth * 2.0f) - 1.0f,
                                       -((screenPos.y / winHeight * 2.0f) - 1.0f), 0.0, 1.0), inv);
    const vec3f camWorldPos = vec3fDiv(vec3f(v.x, v.y, v.z), v.w);


    if(viewIsIso) {
        // inverse rotation
        quat camRot = quatMul(quatAxisRotation(vec3f(0, 0, 1), -RS_HALFPI/2),
                              quatAxisRotation(vec3f(1, 0, 0), VIEW_X_ANGLE));
        camRot = rs_normalize(camRot);
        vec3f camDir = rs_normalize(quatRotateVec3(vec3f(0, 0, -1), camRot));
        f32 d = rs_dot(vec3fMinus(camWorldPos), vec3f(0,0,1)) / rs_dot(camDir, vec3f(0,0,1));
        return vec3fAdd(camWorldPos, vec3fMul(camDir, d));
    }

    return vec3f(camWorldPos.x, camWorldPos.y, 0);
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

static void ImGuiCopyFrameData(RendererFrameData* frame)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    ImDrawList** list = drawData->CmdLists;
    const i32 cmdListsCount = drawData->CmdListsCount;
    frame->imguiDrawList.reserve(cmdListsCount);

    for(i32 i = 0; i < cmdListsCount; ++i) {
        frame->imguiDrawList.push(*list[i]);
        assert(list[i]->VtxBuffer.size() > 0);
    }

    for(i32 i = 0; i < cmdListsCount; i++) {
        assert(frame->imguiDrawList[i].VtxBuffer.size() ==
               list[i]->VtxBuffer.size());
        assert(frame->imguiDrawList[i].VtxBuffer.Data !=
               list[i]->VtxBuffer.Data);
        assert(frame->imguiDrawList[i].VtxBuffer.size() > 0);
    }
}

unsigned long thread_game(void*)
{
    LOG("thread_game started [%x]", threadGetId());

    LOG("Game> initialization...");
    Window& client = *get_clientWindow();
    initTileUVs();

    renderer_waitForInit();

    Game game;
    pGame = &game;

    if(!resource_init()) {
        client.clientRunning = false;
        return 1;
    }

    client.addInputCallback(receiveGameInput, &game);
    game.init();

    dbgDrawInit();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    LOG_SUCC("Game> initializated");

    while(client.clientRunning) {
        auto t0 = timeNow();
        game.frameData.clear();
        game.processInput();
        resource_newFrame();

        game.requestTexBrowserTextures();

#ifdef CONF_ENABLE_UI
        client.dbguiNewFrame();
        game.ui_all();
        client.dbguiFrameEnd();

        ImGuiCopyFrameData(&game.frameData);
#endif

        game.loadSectorIfNeeded();
        game.render();

        renderer_pushFrame(game.frameData);
        game.frameTime = timeDurSince(t0);
    }

    // TODO: wait for deinit
    LOG("Game> cleaning up...");
    game.deinit();

    return 0;
}

RendererFrameData* game_getFrameData()
{
    return &pGame->frameData;
}
