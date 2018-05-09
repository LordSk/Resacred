#pragma once
#include "rs_base.h"
#include "rs_file.h"

struct PakTextureInfo
{
#ifdef CONF_DEBUG
    char name[32];
#endif
    u16 width;
    u16 height;
    PakTextureType type;
};

struct SectorInfo
{
    i32 sectorId;
    i32 fileOffset;
    i32 compressedSize;
    i32 uncompressedSize;
    i32 posX1;
    i32 posX2;
    i32 posY1;
    i32 posY2;
    i32 numHeights;
};

bool resource_init();
void resource_deinit();
void resource_newFrame();

void resource_requestTextures(const i32* textureIds, const i32 textureCount);
void resource_requestGpuTextures(const i32* textureUIDs, u32** out_gpuHandles, const i32 textureCount);
u32 resource_defaultGpuTexture();

SectorxData* resource_loadSector(i32 sectorId);
const SectorInfo& resource_getSectorInfo(i32 sectorId);
u16* resource_getTileTextureIds18(); // Usage: textures[tileId/18]
i32 resource_getTileCount18(); // Real tile count: count * 18

i32 resource_getTextureCount();
PakTextureInfo* resource_getTextureInfos();

FloorEntry* resource_getFloors();
i32 resource_getFloorCount();

PakStatic* resource_getStatic();
i32 resource_getStaticCount();
PakItemType* resource_getItemTypes();
i32 resource_getItemTypesCount();

PakMixedDesc* resource_getMixedDescs();
i32 resource_getMixedDescsCount();
PakMixedData* resource_getMixedData();

void resource_getWorldOrigin(i32* x, i32* y);
