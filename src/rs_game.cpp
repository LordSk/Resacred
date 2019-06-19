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

#include <bgfx/bgfx.h>



// interesting sectors:

// interesting texture page: 27 (minimap)

#define PAGE_TEXTURES_COUNT 160
#define VIEW_X_ANGLE (1.04719755119659774615) // 60 degrees
#define TILE_WIDTH 67.9
#define SECTOR_INVALID_ID (0x0)
#define SECTOR_DRAWN_COUNT 9 // 3 * 3

static vec2 s_tileUV[18][4];

static void initTileUVs()
{
	s_tileUV[0][0] = vec2(0, 24.5/256.f);
	s_tileUV[0][1] = vec2(50.5/256.f, 0/256.f);
	s_tileUV[0][2] = vec2(99.5/256.f, 24.5/256.f);
	s_tileUV[0][3] = vec2(50.5/256.f, 49/256.f);

	s_tileUV[1][0] = vec2(104.5/256.f, 24.5/256.f);
	s_tileUV[1][1] = vec2(153.5/256.f, 0/256.f);
	s_tileUV[1][2] = vec2(204/256.f, 24.5/256.f);
	s_tileUV[1][3] = vec2(153.5/256.f, 49/256.f);

    for(i32 i = 2; i < 18; i += 2) {
        i32 line = i / 2;
        f32 offsetX = (line & 1) * 52/256.f;

		s_tileUV[i][0] = vec2fAdd(s_tileUV[0][0], vec2(offsetX, 25/256.f * line));
		s_tileUV[i][1] = vec2fAdd(s_tileUV[0][1], vec2(offsetX, 25/256.f * line));
		s_tileUV[i][2] = vec2fAdd(s_tileUV[0][2], vec2(offsetX, 25/256.f * line));
		s_tileUV[i][3] = vec2fAdd(s_tileUV[0][3], vec2(offsetX, 25/256.f * line));

		s_tileUV[i+1][0] = vec2fAdd(s_tileUV[1][0], vec2(offsetX, 25/256.f * line));
		s_tileUV[i+1][1] = vec2fAdd(s_tileUV[1][1], vec2(offsetX, 25/256.f * line));
		s_tileUV[i+1][2] = vec2fAdd(s_tileUV[1][2], vec2(offsetX, 25/256.f * line));
		s_tileUV[i+1][3] = vec2fAdd(s_tileUV[1][3], vec2(offsetX, 25/256.f * line));
    }
}

void makeTileMesh(TileVertex* mesh, i32 localTileId, f32 x, f32 y, f32 z, u32 color, i32 alphaMaskTileId = 0)
{
    assert(localTileId >= 0 && localTileId < 18);

    if(alphaMaskTileId) {
        mesh[0] = TileVertex(0.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][1].x, s_tileUV[localTileId][1].y,
				s_tileUV[alphaMaskTileId][1].x, s_tileUV[alphaMaskTileId][1].y,
                color);
        mesh[1] = TileVertex(1.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][2].x, s_tileUV[localTileId][2].y,
				s_tileUV[alphaMaskTileId][2].x, s_tileUV[alphaMaskTileId][2].y,
                color);
        mesh[2] = TileVertex(1.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][3].x, s_tileUV[localTileId][3].y,
				s_tileUV[alphaMaskTileId][3].x, s_tileUV[alphaMaskTileId][3].y,
                color);
        mesh[3] = TileVertex(0.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][1].x, s_tileUV[localTileId][1].y,
				s_tileUV[alphaMaskTileId][1].x, s_tileUV[alphaMaskTileId][1].y,
                color);
        mesh[4] = TileVertex(1.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][3].x, s_tileUV[localTileId][3].y,
				s_tileUV[alphaMaskTileId][3].x, s_tileUV[alphaMaskTileId][3].y,
                color);
        mesh[5] = TileVertex(0.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][0].x, s_tileUV[localTileId][0].y,
				s_tileUV[alphaMaskTileId][0].x, s_tileUV[alphaMaskTileId][0].y,
                color);
    }
    else {
        mesh[0] = TileVertex(0.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][1].x, s_tileUV[localTileId][1].y,
                -1, -1,
                color);
        mesh[1] = TileVertex(1.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][2].x, s_tileUV[localTileId][2].y,
                -1, -1,
                color);
        mesh[2] = TileVertex(1.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][3].x, s_tileUV[localTileId][3].y,
                -1, -1,
                color);
        mesh[3] = TileVertex(0.0 + x, 0.0 + y, z,
				s_tileUV[localTileId][1].x, s_tileUV[localTileId][1].y,
                -1, -1,
                color);
        mesh[4] = TileVertex(1.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][3].x, s_tileUV[localTileId][3].y,
                -1, -1,
                color);
        mesh[5] = TileVertex(0.0 + x, 1.0 + y, z,
				s_tileUV[localTileId][0].x, s_tileUV[localTileId][0].y,
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

struct Rect
{
    vec2 rmin;
    vec2 rmax;
};

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
f32 viewZ = 0;
f32 viewZoom = 1.0f;
i32 viewZMul = -1;

SectorxData* sectorData = nullptr;
SectorInfo sectorInfo;

u16 sectorIdMap[10000];
Rect camWorldRect;
vec2 camWorldGridPos;

// sector draw data
struct SectorDrawData
{
    Array<i32> pakTextureIds;
    Array<TileVertex> tileVertexData;
    i32 floorCount = 0;
    i32 mixedQuadCount = 0;

    inline void clear() {
        pakTextureIds.clearPOD();
        tileVertexData.clearPOD();

        floorCount = 0;
        mixedQuadCount = 0;
    }
};

i32 sectorIdToLoad[SECTOR_DRAWN_COUNT];
i32 sectorIdLoaded[SECTOR_DRAWN_COUNT] = {0};
SectorDrawData sectorDrawData[SECTOR_DRAWN_COUNT];

bool showUi = true;
bool dbgShowFrameGraph = false;
i32 dbgSectorId = 3810;

i32 dbgFloorOffset = 0;
bool dbgFloorOver = 0;
i32 dbgOverlayFloor = 1;

i32 dbgMixedId = 0;
bool dbgShowMixed = true;
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

inline vec3 posOrthoToIso(vec3 v)
{
    return vec3fMulMat4(v, matIsoRotation);
}

inline vec3 sacred_worldToScreen(vec3 v)
{
	return vec3(v.x * 0.89442718 + v.y * -0.89442718, v.x * 0.44721359 + v.y * 0.44721359, v.z);
}

inline vec3 sacred_screenToWorld(vec3 v)
{
	return vec3((v.x * 0.44721359 - v.y * -0.89442718)/* * 1.25*/, (v.y * 0.89442718 - v.x * 0.44721359)/* * 1.25*/, v.z);
}

void init()
{
    Window& client = *get_clientWindow();
    winWidth = client.width;
    winHeight = client.height;

    matProjOrtho = mat4Orthographic(0, winWidth, winHeight, 0, -1.0f, 10000.f);
    matIsoRotation = mat4RotateAxisX(VIEW_X_ANGLE);
    matIsoRotation = mat4Mul(matIsoRotation, mat4RotateAxisZ(-RS_HALFPI/2));

    // setup sectorIdMap (100x100)
    memset(sectorIdMap, 0x00, sizeof(sectorIdMap)); // fill with invalid ids
    const i32 sectorCount = resource_getSectorCount();
    const SectorInfo*__restrict sectorInfoList = resource_getSectorInfoList();
    for(i32 i = 0; i < sectorCount; ++i) {
        const SectorInfo& si = sectorInfoList[i];
        assert(si.sectorId >= 0 && si.sectorId < 6051);
        const i32 gridX = (((si.posX1 + 25) / 53.66563) / 64);
        const i32 gridY = (((si.posY1 + 25) / 53.66563) / 64);
        assert(gridX >= 0 && gridX < 100);
        assert(gridY >= 0 && gridY < 100);
        assert(sectorIdMap[gridY * 100 + gridX] == SECTOR_INVALID_ID);
        sectorIdMap[gridY * 100 + gridX] = si.sectorId;
    }

	initTileUVs();
}

void loadSectorsIfNeeded()
{
    static Array<i32,4096> baseTileId;
    static Array<i32,4096> baseTexId;
    static Array<i32> floorDiffuseTileId;
    static Array<i32> floorAlphaTileId;
    static Array<i32> floorDiffuseTexId;
    static Array<i32> floorAlphaTexId;
    static Array<i32> floorPosIndex;
    static Array<i32> mixedTexId;

    static Array<TileVertex> baseVertexData;
    static Array<TileVertex> floorVertexData;
    static Array<TileVertex> mixedVertexData;

    frameData.doUploadTileVertexData = true;

    for(i32 s = 0; s < SECTOR_DRAWN_COUNT; ++s) {
        if(sectorIdToLoad[s] == SECTOR_INVALID_ID ||
           sectorIdToLoad[s] == sectorIdLoaded[s]) {
            continue;
        }

        baseTileId.clearPOD();
        baseTexId.clearPOD();
        floorDiffuseTileId.clearPOD();
        floorAlphaTileId.clearPOD();
        floorDiffuseTexId.clearPOD();
        floorAlphaTexId.clearPOD();
        floorPosIndex.clearPOD();
        mixedTexId.clearPOD();

        baseVertexData.clearPOD();
        floorVertexData.clearPOD();
        mixedVertexData.clearPOD();

        const i32 sectorId = sectorIdToLoad[s];
        sectorData = resource_loadSector(sectorId);
        sectorInfo = resource_getSectorInfo(sectorId);

        SectorDrawData& dd = sectorDrawData[s];
        dd.clear();

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

        const i32 sectorX = sectorInfo.posX1;
        const i32 sectorY = sectorInfo.posY1;
        const vec3 sectorSceen = sacred_worldToScreen(vec3(sectorX, sectorY, 0));

        TileVertex quad[6];

        // 64 * 64
        for(i32 i = 0; i < 4096; ++i) {
            const WldxEntry& we = sectorEntries[i];
            const i32 tileId = sectorEntries[i].tileId;
            const i32 texId = tileTexIds[tileId/18];
            assert(tileId < tileCount);
            baseTexId.push(texId);
            baseTileId.push(tileId);

            i32 floorId = we.floorId;
            while(floorId && dbgOverlayFloor) {
                assert(floorId < floorMaxCount);
                const i32 diffuseTileId = floors[floorId].pakTileIds & 0x1FFFF;
                const i32 tileIdAlphaMask = floors[floorId].pakTileIds >> 17;

                if(diffuseTileId > 0) {
                    dd.floorCount++;
                    floorDiffuseTileId.push(diffuseTileId);
                    floorDiffuseTexId.push(tileTexIds[diffuseTileId/18]);
                    floorAlphaTileId.push(tileIdAlphaMask);
                    floorAlphaTexId.push(tileTexIds[tileIdAlphaMask/18]);
                    floorPosIndex.push(i);
                }
                floorId = floors[floorId].nextFloorId;
            }

            if(we.staticId) {
                assert(we.staticId < staticCount);
                const PakStatic& sta = statics[we.staticId];
                i32 itemTypeId = sta.itemTypeId;

                if(itemTypeId) {
                    assert(itemTypeId < itemTypeCount);
                    const i32 mixedId = itemTypes[itemTypeId].mixedId;

                    if(mixedId) {
                        assert(mixedId < mixedDescCount);
                        const PakMixedDesc& desc = mixedDescs[mixedId];
                        const i32 mcount = desc.count;
                        const i32 mixedStartId = desc.mixedDataId;

                        f32 orgnX = sta.worldX - sectorSceen.x;
                        f32 orgnY = sta.worldY - sectorSceen.y;
                        mat4 inv = mat4Inv(matIsoRotation);
                        vec3 orgnPosIso = vec3fMulMat4(vec3(orgnX, orgnY, 0), inv);

                        if(viewIsIso) {
                            orgnPosIso = posOrthoToIso(orgnPosIso);
                        }

                        for(i32 m = 0; m < mcount; ++m) {
                            const PakMixedData& md = mixed[m + mixedStartId];
                            f32 x = orgnPosIso.x + md.x;
                            f32 y = orgnPosIso.y + md.y;
                            i32 w = md.width - md.x;
                            i32 h = md.height - md.y;
                            meshAddQuad(quad, x, y, 0.0, w, h, md.uvX1, md.uvY1,
                                        md.uvX2, md.uvY2, 0xffffffff);

                            mixedTexId.push(md.textureId);
                            mixedVertexData.pushPOD(quad, 6);
                            dd.mixedQuadCount++;
                        }
                    }
                }
            }
        }

        for(i32 y = 0; y < 64; ++y) {
            for(i32 x = 0; x < 64; ++x) {
                const i32 id = y * 64 + x;
                makeTileMesh(quad, baseTileId[id] % 18, x, y, 0.0, 0xffffffff);
                baseVertexData.pushPOD(quad, 6);
            }
        }

        for(i32 i = 0; i < dd.floorCount; ++i) {
            const i32 posIndx = floorPosIndex[i];
            const i32 x = posIndx & 63;
            const i32 y = posIndx / 64;
            makeTileMesh(quad, floorDiffuseTileId[i] % 18, x, y, 0.0, 0xffffffff,
                         floorAlphaTileId[i] % 18);
            floorVertexData.pushPOD(quad, 6);
        }

        dd.tileVertexData.pushPOD(baseVertexData.data(), baseVertexData.count());
        dd.tileVertexData.pushPOD(floorVertexData.data(), floorVertexData.count());
        dd.tileVertexData.pushPOD(mixedVertexData.data(), mixedVertexData.count());

        dd.pakTextureIds.pushPOD(baseTexId.data(), baseTexId.count());
        dd.pakTextureIds.pushPOD(floorDiffuseTexId.data(), floorDiffuseTexId.count());
        dd.pakTextureIds.pushPOD(floorAlphaTexId.data(), floorAlphaTexId.count());
        dd.pakTextureIds.pushPOD(mixedTexId.data(), mixedTexId.count());

        sectorIdLoaded[s] = sectorId;
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
    VramInfo vi = renderer_getVramInfo();
    ImGui::SetNextWindowPos(ImVec2(5,5));
    ImGui::Begin("Video info", nullptr,
                 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
                 ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Current/Total %dmb/%dmb", (vi.availMemory-vi.currentAvailMem)/1024, vi.availMemory/1024);
    ImGui::ProgressBar(1.0 - (vi.currentAvailMem / (f64)vi.availMemory));

    ImGui::Separator();

    ImGui::Text("Renderer frametime: %.3fms", renderer_getFrameTime() * 1000.0);
    ImGui::Text("Game     frametime: %.3fms", frameTime * 1000.0);
    ImGui::Checkbox("Show frame graph", &dbgShowFrameGraph);

    ImGui::End();
}

void ui_frameGraph()
{
    static f32 rdrFtStack[1000] = {0};
    static f32 gameFtStack[1000] = {0};
    memmove(rdrFtStack + 1, rdrFtStack, sizeof(rdrFtStack) - sizeof(rdrFtStack[0]));
    memmove(gameFtStack + 1, gameFtStack, sizeof(gameFtStack) - sizeof(gameFtStack[0]));
    rdrFtStack[0] = renderer_getFrameTime() * 1000.0;
    gameFtStack[0] = frameTime * 1000.0;

    if(!dbgShowFrameGraph) return;

    ImGui::Begin("Frame graph");

    const i32 count = arr_count(rdrFtStack);
    const f32 width = ImGui::GetWindowContentRegionWidth();

    f32 rdrMin = 1000.0, rdrMax = 0.0;
    f32 gameMin = 1000.0, gameMax = 0.0;

    for(i32 i = 0; i < count; ++i) {
		rdrMin = MIN(rdrFtStack[i], rdrMin);
		rdrMax = MAX(rdrFtStack[i], rdrMax);
		gameMin = MIN(gameFtStack[i], gameMin);
		gameMax = MAX(gameFtStack[i], gameMax);
    }

    ImGui::Text("Renderer frametime .. [%.3f, %.3f] (ms)", rdrMin, rdrMax);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xffff6100);
    ImGui::PlotHistogram("##render_ft_hist", rdrFtStack, count, 0, 0, rdrMin, rdrMax, ImVec2(width,80));
    ImGui::PopStyleColor(1);

    ImGui::Text("Game frametime     .. [%.3f, %.3f] (ms)", gameMin, gameMax);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xff0061ff);
    ImGui::PlotHistogram("##game_ft_hist", gameFtStack, count, 0, 0, gameMin, gameMax, ImVec2(width,80));
    ImGui::PopStyleColor(1);

    ImGui::End();
}

void ui_all()
{
    if(!showUi) return;

    ui_textureBrowser();
    ui_tileTest();
    ui_mixedViewer();
    ui_videoInfo();
    ui_frameGraph();

    ui_tileInspector();

	resources_debugUi();

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

    ImGui::SliderInt("Sector ID", &dbgSectorId, 1, 6049);
    ImGui::InputInt("##sector_id_input", &dbgSectorId);

    //ImGui::SliderInt("viewZ", &dbgViewZ, -100000, 100000);
    ImGui::Text("Cam world grid pos: {%d, %d}", (i32)camWorldGridPos.x, (i32)camWorldGridPos.y);
    ImGui::SliderFloat("TILE_WIDTH", &dbgTileWidth, 1, 200, "%.1f");
    ImGui::SliderInt("Mixed max objects drawn", &dbgMixedObjMax, 0, 4096);


    dbgSectorId = clamp(dbgSectorId, 0, 6049);

    ImGui::SliderInt("Overlay floors", &dbgOverlayFloor, 0, 2);
    ImGui::Checkbox("Show mixed", &dbgShowMixed);

    if(sectorData) {
        ImGui::Text("#%d (%s)", dbgSectorId, sectorData->name);
        ImGui::Text("posX1: %d posX2: %d", sectorInfo.posX1, sectorInfo.posX2);
        ImGui::Text("posY1: %d posY2: %d", sectorInfo.posY1, sectorInfo.posY2);

        const i32 gridX = ((sectorInfo.posX1 + 25) / 53.66563) / 64;
        const i32 gridY = ((sectorInfo.posY1 + 25) / 53.66563) / 64;
        ImGui::Text("grid: {%d, %d}", gridX, gridY);

        vec3 sectorPos(sectorInfo.posX1, sectorInfo.posY1, 0);

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

        if(ImGui::Button("Dump data to file")) {
            char path[256];
            sprintf(path, "sector_data.%d", dbgSectorId);
            fileWriteBuffer(path, (const char*)sectorData, sectorInfo.uncompressedSize);
        }
    }

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
    if(dbgSelectedTileId == -1) return;
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

			auto unkElementGet = [](void* data, i32 id, const char** outStr) -> bool {
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

        vec3 posIso = sacred_screenToWorld(vec3(s.worldX, s.worldY, 0));
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
    frameData.viewIsIso = viewIsIso;
    frameData.matCamProj = matProjOrtho;
    frameData.matCamViewIso = matViewIso;
    frameData.matCamViewOrtho = matViewOrtho;

    // sector grid
    for(i32 y = 0; y < 100; ++y) {
        for(i32 x = 0; x < 100; ++x) {
            const i32 id = y * 100 + x;
            if(sectorIdMap[id] != SECTOR_INVALID_ID && sectorIdMap[id] != sectorIdLoaded[0]) {
                dbgDrawSolidSquare(vec3(x * dbgTileWidth * 64, y * 64 * dbgTileWidth, 0),
                                   vec3(dbgTileWidth * 64, dbgTileWidth * 64, 1),
                                   0x8f8f1f1f);
            }
        }
    }

    for(i32 x = 0; x < 101; ++x) {
        dbgDrawSolidSquare(vec3(x * 64 * dbgTileWidth, 0, 0), vec3(2*viewZoom, dbgTileWidth * 64 * 100, 1),
                           0xff8f8f8f);
    }
    for(i32 y = 0; y < 101; ++y) {
        dbgDrawSolidSquare(vec3(0, y * 64 * dbgTileWidth, 0), vec3(dbgTileWidth * 64 * 100, 2*viewZoom, 1),
                           0xff8f8f8f);
    }

    if(sectorIdLoaded[0] != SECTOR_INVALID_ID) {
        const SectorDrawData& dd = sectorDrawData[0];

        frameData.tileVertexData.pushPOD(dd.tileVertexData.data(), dd.tileVertexData.count());
        frameData.tileQuadGpuTex.fillPOD(dd.pakTextureIds.count());

        resource_requestGpuTextures(dd.pakTextureIds.data(), frameData.tileQuadGpuTex.data(),
                                    dd.pakTextureIds.count());

        const vec2 sectorWorldPos = camWorldGridPos * 64 * dbgTileWidth;
        frameData.matSectorTileModel = mat4Translate(vec3(sectorWorldPos.x, sectorWorldPos.y, 0)) *
                                       mat4Scale(vec3(dbgTileWidth, dbgTileWidth, 1));

        if(viewIsIso) {
            frameData.matSectorMixedModel = mat4Translate(vec3(sectorWorldPos.x, sectorWorldPos.y, 0) *
                                                          matIsoRotation);
        }
        else {
            frameData.matSectorMixedModel = mat4Translate(vec3(sectorWorldPos.x, sectorWorldPos.y, 0));
        }

        frameData.tvOff_base = 0;
        frameData.tvOff_floor = 4096 * 6;
        frameData.tvOff_mixed = frameData.tvOff_floor + dd.floorCount * 6;
    }

    // TODO: restore
#if 0
    // debug tiles
    const WldxEntry* sectorEntries = sectorData->data;
    for(i32 y = 0; y < 64; ++y) {
        for(i32 x = 0; x < 64; ++x) {
            const i32 id = y * 64 + x;
            const WldxEntry& we = sectorEntries[id];
            u32 color = 0xffffffff;

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

            if(color != 0xffffffff) {
                dbgDrawSolidSquare(vec3(x*dbgTileWidth, y*dbgTileWidth, 0),
                                   vec3(dbgTileWidth, dbgTileWidth, 0), color,
                                   DbgCoordSpace::WORLD);
            }
        }
    }

    if(dbgHoveredTileId != -1) {
        const f32 selX = (dbgHoveredTileId % 64) * dbgTileWidth + sectorWorldPos.x;
        const f32 selY = (dbgHoveredTileId / 64) * dbgTileWidth + sectorWorldPos.y;
        dbgDrawSolidSquare(vec3(selX, selY, 0),
                           vec3(dbgTileWidth, dbgTileWidth, 0), 0xffff8f8f,
                           DbgCoordSpace::WORLD);
    }

    if(dbgSelectedTileId != -1) {
        const f32 selX = (dbgSelectedTileId % 64) * dbgTileWidth + sectorWorldPos.x;
        const f32 selY = (dbgSelectedTileId / 64) * dbgTileWidth + sectorWorldPos.y;
        dbgDrawSolidSquare(vec3(selX, selY, 0),
                           vec3(dbgTileWidth, dbgTileWidth, 0), 0xffff0000,
                           DbgCoordSpace::WORLD);
    }
#endif
}

void updateCamera()
{
    matProjOrtho = mat4Orthographic(0, winWidth, winHeight, 0, -100000.f, 100000.f);
    matViewOrtho = mat4Mul(
                       mat4Scale(vec3(1.0f/viewZoom, 1.0f/viewZoom, 1)),
                       mat4Translate(vec3(-viewX, -viewY, -viewZ))
                       );
    matViewIso = mat4Mul(matViewOrtho, matIsoRotation);

    // frustrum
    mat4 mvp;
    if(viewIsIso) {
        mvp = mat4Mul(matProjOrtho, matViewIso);
    }
    else {
        mvp = mat4Mul(matProjOrtho, matViewOrtho);
    }

    const mat4 inv = mat4Inv(mvp);


    const vec2 corners[2] = {
        vec2(-1, -1),
        vec2(1, 1),
    };
    vec2 worldCorners[2];

    for(i32 i = 0; i < 2; ++i) {
        const vec4 v = vec4fMulMat4(vec4(corners[i].x, corners[i].y, 0.0, 1.0), inv);
        vec3 camWorldPos = vec3fDiv(vec3(v.x, v.y, v.z), v.w);

        if(viewIsIso) {
            // inverse rotation
            quat camRot = quatMul(quatAxisRotation(vec3(0, 0, 1), -RS_HALFPI/2),
                                  quatAxisRotation(vec3(1, 0, 0), VIEW_X_ANGLE));
            camRot = rs_normalize(camRot);
            vec3 camDir = rs_normalize(quatRotateVec3(vec3(0, 0, -1), camRot));
            f32 d = rs_dot(vec3fMinus(camWorldPos), vec3(0,0,1)) / rs_dot(camDir, vec3(0,0,1));
            camWorldPos = vec3fAdd(camWorldPos, vec3fMul(camDir, d));
        }

        worldCorners[i] = vec2(camWorldPos.x, camWorldPos.y);
    }

    camWorldRect.rmin = worldCorners[0];
    camWorldRect.rmax = worldCorners[1];
    camWorldGridPos = (camWorldRect.rmin + camWorldRect.rmax) * 0.5 / dbgTileWidth / 64;
    camWorldGridPos.x = (i32)camWorldGridPos.x;
    camWorldGridPos.y = (i32)camWorldGridPos.y;

    if(viewIsIso) {
        vec2 pos((camWorldRect.rmin.x + camWorldRect.rmax.x) * 0.5, camWorldRect.rmax.y);
        viewZ = (vec3(pos.x, pos.y, 0) * matIsoRotation).z;
    }
    else {
        viewZ = 0;
    }
}

void update()
{
    updateCamera();


    const i32 gridX = camWorldGridPos.x;
    const i32 gridY = camWorldGridPos.y;
    const u16 sectorId = sectorIdMap[gridY * 100 + gridX];
    sectorIdToLoad[0] = sectorId;
    sectorIdToLoad[1] = sectorId;
    sectorIdToLoad[2] = sectorId;
    sectorIdToLoad[3] = sectorId;
    sectorIdToLoad[4] = sectorId;
    sectorIdToLoad[5] = sectorId;
    sectorIdToLoad[6] = sectorId;
    sectorIdToLoad[7] = sectorId;
    sectorIdToLoad[8] = sectorId;

    loadSectorsIfNeeded();

    // select tile to inspect
    const vec2 mpos(input.mouseX, input.mouseY);
    const vec3 mWorldPos = screenToWorldPos(mpos); // actual world position

    const i32 tx = mWorldPos.x / dbgTileWidth;
    const i32 ty = mWorldPos.y / dbgTileWidth;
    if(tx >= 0 && tx < 64 && ty >= 0 && ty < 64) {
        dbgHoveredTileId = ty * 64 + tx;

        if(!ImGui::GetIO().WantCaptureMouse && input.mouseLeft) {
            dbgSelectedTileId = dbgHoveredTileId;
        }
    }



    drawSector();

#if 1
    dbgDrawSetView(matProjOrtho, mat4Identity(), DbgCoordSpace::SCREEN);
    dbgDrawSetView(matProjOrtho, viewIsIso ? matViewIso : matViewOrtho, DbgCoordSpace::WORLD);

    // origin
    dbgDrawSolidSquare(vec3(0,0,0), vec3(200, 10, 1), 0xffff0000, DbgCoordSpace::WORLD);
    dbgDrawSolidSquare(vec3(0,0,0), vec3(10, 200, 1), 0xff00ff00, DbgCoordSpace::WORLD);
    dbgDrawSolidSquare(vec3(0,0,0), vec3(10, 10, 1), 0xff0000ff, DbgCoordSpace::WORLD);

    dbgDrawSetFrameData(&frameData);
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
vec3 screenToWorldPos(const vec2 screenPos)
{
    mat4 mvp;
    if(viewIsIso) {
        mvp = mat4Mul(matProjOrtho, matViewIso);
    }
    else {
        mvp = mat4Mul(matProjOrtho, matViewOrtho);
    }

    const mat4 inv = mat4Inv(mvp);
    const vec4 v = vec4fMulMat4(vec4((screenPos.x / winWidth * 2.0f) - 1.0f,
                                       -((screenPos.y / winHeight * 2.0f) - 1.0f), 0.0, 1.0), inv);
    const vec3 camWorldPos = vec3fDiv(vec3(v.x, v.y, v.z), v.w);


    if(viewIsIso) {
        // inverse rotation
        quat camRot = quatMul(quatAxisRotation(vec3(0, 0, 1), -RS_HALFPI/2),
                              quatAxisRotation(vec3(1, 0, 0), VIEW_X_ANGLE));
        camRot = rs_normalize(camRot);
        vec3 camDir = rs_normalize(quatRotateVec3(vec3(0, 0, -1), camRot));
        f32 d = rs_dot(vec3fMinus(camWorldPos), vec3(0,0,1)) / rs_dot(camDir, vec3(0,0,1));
        return vec3fAdd(camWorldPos, vec3fMul(camDir, d));
    }

    return vec3(camWorldPos.x, camWorldPos.y, 0);
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
	// FIXME: reenable
	/*frame->imguiDrawList.reserve(cmdListsCount);

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
	}*/
}

unsigned long thread_game(void*)
{
    LOG("thread_game started [%x]", threadGetId());

    LOG("Game> initialization...");
    Window& client = *get_clientWindow();

	// NOTE: must be called on this thread (bgfx::init and bgfx::frame need to be on the same thread)
	if(!renderer_init()) {
		LOG("ERROR: could not init renderer"); // TODO: error display here maybe?
		return 1;
	}

	Game game;
	pGame = &game;

	if(!resource_init()) {
		LOG("ERROR: could not init resources");
		client.isRunning = false;
		return 1;
	}

	while(client.isRunning) {
		bgfx::touch(0);

		bgfx::dbgTextClear();
		bgfx::dbgTextPrintf(0, 0, 0x0d, "Resacred by LordSk");
		bgfx::dbgTextPrintf(0, 1, 0x0f, "Unicorn Multi-shot OP");

		const bgfx::Stats* stats = bgfx::getStats();
		bgfx::dbgTextPrintf(0, 2, 0x0e, "GPU Memory: [%.1f%%] %llu / %llu (MB)", stats->gpuMemoryUsed / (f64)stats->gpuMemoryMax, stats->gpuMemoryUsed/(1024*1024), stats->gpuMemoryMax/(1024*1024));

		client.dbgUiNewFrame();

		ImGui::ShowDemoWindow();
		resources_debugUi();

		renderer_renderDbgUi();
		renderer_frame();

		client.swapBuffers();
	}

	/*client.addInputCallback(receiveGameInput, &game);
    game.init();

    dbgDrawInit();

    //pak_FloorRead("../sacred_data/Floor.pak");
    //bin_WorldRead("../sacred_data/World.bin");

    LOG_SUCC("Game> initializated");

    while(client.clientRunning) {
        auto t0 = timeNow();
        game.processInput();
        resource_newFrame();

        game.requestTexBrowserTextures();

#ifdef CONF_ENABLE_UI
		client.dbgUiNewFrame();
        game.ui_all();
		client.dbgUiFrameEnd();

        ImGuiCopyFrameData(&game.frameData);
#endif

        game.update();

        game.frameTime = timeDurSince(t0);
        renderer_pushFrame(game.frameData);
        game.frameData.clear();
    }

    // TODO: wait for deinit
    LOG("Game> cleaning up...");
    game.deinit();
*/
    return 0;
}

RendererFrameData* game_getFrameData()
{
    return &pGame->frameData;
}
