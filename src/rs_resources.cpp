#include "rs_resources.h"
#include "rs_renderer.h"
#include "rs_file.h"
#include "rs_game.h"

// TODO: move all file operations out of here?
// TODO: Load a range of textures instead of just one

/*
 * GPU resource manager
 *
 * Usage:
 *  - request textures every frame
 *  - get a pointer back, dereference to get actual gpu handle
 *  - points to zero while texture is being uploaded to gpu
 *
 *  - textures not being requested for a while will be destroyed
 */

struct GPUResources
{
u32 gpuTexDefault;
u32 texGpuId[MAX_GPU_TEXTURES];
i32 texDiskId[MAX_GPU_TEXTURES];
i32 texFramesNotRequested[MAX_GPU_TEXTURES];
AtomicCounter texLoaded[MAX_GPU_TEXTURES];
u8 texSlotOccupied[MAX_GPU_TEXTURES];
TextureDesc2D texDesc[MAX_GPU_TEXTURES];
RendererFrameData* frameData;

bool init()
{
    memset(texFramesNotRequested, 0, sizeof(texFramesNotRequested));
    memset(texSlotOccupied, 0, sizeof(texSlotOccupied));
    memset(texLoaded, 0, sizeof(texLoaded));

    constexpr i32 texWidth = 16;
    static u32 textureData[texWidth*texWidth];

    for(i32 i = 0; i < texWidth*texWidth; ++i) {
        i32 odd = (i/texWidth) & 1;
        if((i+odd) & 1) {
            textureData[i] = 0xffff0000;
        }
        else {
            textureData[i] = 0xffff00ff;
        }
    }

    TextureDesc2D desc;
    desc.internalFormat = GL_RGBA8;
    desc.dataFormat = GL_RGBA;
    desc.dataPixelCompType = GL_UNSIGNED_BYTE;
    desc.data = textureData;
    desc.height = texWidth;
    desc.width = texWidth;
    desc.wrapS = GL_REPEAT;
    desc.wrapT = GL_REPEAT;
    desc.minFilter = GL_NEAREST;
    desc.magFilter = GL_NEAREST;

    frameData = game_getFrameData();
    frameData->_addTextureCreate(desc, &gpuTexDefault);

    return true;
}

void destroyTexture(i32 texSlot)
{
    if(texGpuId[texSlot] != gpuTexDefault) {
        frameData->_addTextureToDestroyList(texGpuId[texSlot]);
    }
    texGpuId[texSlot] = gpuTexDefault;
    texDiskId[texSlot] = 0;
    texLoaded[texSlot].set(0);
    texSlotOccupied[texSlot] = false;
}

void newFrame()
{
    frameData = game_getFrameData();

    // find unused textures and destroy them
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i]) {
            texFramesNotRequested[i]++;

            if(texFramesNotRequested[i] > 10) {
                destroyTexture(i);
                //LOG_DBG("ResourceGpu> evicting texture %d", i);
            }
        }
    }
}

i32 _occupyNextTextureSlot()
{
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i] == false) {
            texSlotOccupied[i] = true;
            return i;
        }
    }

    LOG_WARN("ResourceGpu> Warning, requesting a lot of textures quickly");

    i32 oldestFrameCount = 0;
    i32 oldestId = -1;
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i] && texFramesNotRequested[i] > oldestFrameCount &&
           texLoaded[i].get() == 1) {
            oldestId = i;
            oldestFrameCount = texFramesNotRequested[i];
        }
    }

    assert(oldestId >= 0);
    assert(oldestFrameCount > 0);

    destroyTexture(oldestId);
    texSlotOccupied[oldestId] = true;
    return oldestId;
}

void requestTextures(const i32* inPakTextureIds, u32** outGpuTexHandles, const i32 requestCount)
{
    memset(outGpuTexHandles, 0, sizeof(outGpuTexHandles[0]) * requestCount);

    // assign already loaded/loading textures
    for(i32 r = 0; r < requestCount; ++r) {
        const i32 pakTexId = inPakTextureIds[r];

        bool found = false;
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i] && texDiskId[i] == pakTexId) {
                outGpuTexHandles[r] = &texGpuId[i];
                texFramesNotRequested[i] = 1;
                found = true;
                break;
            }
        }

        if(!found) {
            i32 newId = _occupyNextTextureSlot();
            outGpuTexHandles[r] = &texGpuId[newId];
            texGpuId[newId] = gpuTexDefault;
            texDiskId[newId] = pakTexId;
            texFramesNotRequested[newId] = 1;
            // TODO: it increments here but decrements in ResourceManager?
            // choose one
            texLoaded[newId].set(0);
        }
    }
}

void uploadTextures(i32* pakTextureUIDs, PakTextureInfo* textureInfos, u8** textureData, const i32 count)
{
    // for all remaining requests create a new gpu texture and assign it
    for(i32 r = 0; r < count; ++r) {
        const i32 pakTexId = pakTextureUIDs[r];
        i32 gpuId = -1;

        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i] && texDiskId[i] == pakTexId) {
                gpuId = i;
                break;
            }
        }

        if(gpuId == -1 || texLoaded[gpuId].get()) {
            continue;
        }

        //LOG_DBG("ResourcesGpu> uploading pakId=%d gpuId=%d", pakTexId, gpuId);

        TextureDesc2D& desc = texDesc[gpuId];

        // upload texture to gpu
        desc.width = textureInfos[r].width;
        desc.height = textureInfos[r].height;
        desc.data =  textureData[r];

        if(textureInfos[r].type == PakTextureType::TYPE_RGBA8) {
            desc.internalFormat = GL_RGBA8;
            desc.dataFormat = GL_RGBA;
            desc.dataPixelCompType = GL_UNSIGNED_BYTE;
        }
        else {
            desc.internalFormat = GL_RGBA4;
            desc.dataFormat = GL_BGRA;
            desc.dataPixelCompType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        }


        desc.minFilter = GL_NEAREST;
        desc.magFilter = GL_NEAREST;


        texLoaded[gpuId].increment();
        frameData->_addTextureCreate(desc, &texGpuId[gpuId]);
    }
}

void deinit()
{
    // TODO: implement
    /*for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i]) {
            cmds.destroyTexture(texGpuId[i]);
        }
    }*/
}

/*void debugUi()
{
    ImGui::Begin("Gpu resources");

    ImGui::Text("MAX_TEXTURE_IDS: %d", MAX_GPU_TEXTURES);

    ImGui::BeginChild("gpures_texture_list");
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        ImColor textColor(128, 255, 128, 255);
        if(!texSlotOccupied[i]) {
            textColor = ImColor(128, 0, 0, 255);
        }
        ImGui::TextColored(textColor, "[%4d] gpuId=%4d diskId=%4d frames=%4d", i,
            texGpuId[i], texDiskId[i], texFramesNotRequested[i]);
    }
    ImGui::End();

    ImGui::End();
}*/
};

#define FILE_RING_BUFFER_SIZE Megabyte(200)
#define TEXTURE_DATA_SIZE Megabyte(300)
#define TEXTURE_TEMP_SIZE Megabyte(5)
#define FRAME_MAX_PROCESSING_COUNT 24

/*
 * Resource manager
 *
 * Texture request process:
 * - if not loaded, request file read
 */

struct ResourceManager {

DiskFile fileTexture;
i32 textureCount;
i32* textureFileOffset;
i32* textureFileSize;
AtomicCounter* textureDiskLoadStatus; // 2 == loading from disk, 1 == loaded
MemBlock* textureDataBlock;
PakTextureInfo* textureInfo;
u8* textureGpuUpload;
i32* textureAge; // in frames
MemBlock textureMetaBlock;

u8* fileData;
u8* textureRawData;
MemBlock textureFileAndRawBlock;
intptr_t fileRingCursor = 0;
AllocatorBucket textureDataAllocator;

u8* tempTextureBuff;
MemBlock tempTextureBuffBlock;

GPUResources gpu;

u16* tileTextureId;
i32 tileTextureIdCount;
MemBlock tileTextureIdBlock;

i32 worldOriginX;
i32 worldOriginY;
SectorInfo* sectorInfo;
MemBlock sectorInfoBlock;
DiskFile fileSectors;

i32 sectorCount = 0;
MemBlock sectorDataBlock;
AllocatorRing sectorRingAlloc;

Array<i32,64> _pakTexIdUpload;
Array<u8*,64> _dataUpload;
Array<PakTextureInfo,64> _texInfoUpload;

MemBlock floorData;
FloorEntry* floors;
//const i32 floorCount = 10000;
i32 floorEntryCount;

Array<PakStatic> statics;
Array<PakItemType> itemTypes;

PakMixedFileData mixed;

enum class LoadStatus: i32 {
    NONE = 0,
    PROCESSED,
    FILE_LOADED,
    FILE_LOADING,
};

bool init()
{
    gpu.init();

    if(!fileOpenToRead("../sacred_data/texture.pak", &fileTexture)) {
        return false;
    }

    // Setup texture related stuff
    textureFileAndRawBlock = MEM_ALLOC(TEXTURE_DATA_SIZE + FILE_RING_BUFFER_SIZE);
    assert(textureFileAndRawBlock.ptr);
    textureRawData = (u8*)textureFileAndRawBlock.ptr;
    fileData = textureRawData + TEXTURE_DATA_SIZE;

    LOG_DBG("Resource> textureFileAndRawBlock size=%lldmb",
            (TEXTURE_DATA_SIZE + FILE_RING_BUFFER_SIZE)/(1024*1024));

    MemBlock textureRawDataBlock;
    textureRawDataBlock.ptr = textureRawData;
    textureRawDataBlock.size = TEXTURE_DATA_SIZE;
    textureDataAllocator.init(textureRawDataBlock, TEXTURE_DATA_SIZE/1024, 1024);

    tempTextureBuffBlock = MEM_ALLOC(TEXTURE_TEMP_SIZE);
    assert(tempTextureBuffBlock.ptr);
    tempTextureBuff = (u8*)tempTextureBuffBlock.ptr;

    PakHeader header;
    fileReadAdvance(&fileTexture, sizeof(header), &header);
    textureCount = header.entryCount;

    // alloc texture metadata
    textureMetaBlock = MEM_ALLOC((sizeof(*textureFileOffset) + sizeof(*textureFileSize) +
            sizeof(*textureDiskLoadStatus) +
            sizeof(*textureDataBlock) + sizeof(*textureInfo) + sizeof(*textureGpuUpload) +
            sizeof(*textureAge)) * textureCount);
    assert(textureMetaBlock.ptr);

    LOG_DBG("Resource> textureMetaBlock size=%lldkb", textureMetaBlock.size/1024);

    textureFileOffset = (i32*)textureMetaBlock.ptr;
    textureFileSize = (i32*)(textureFileOffset + textureCount);
    textureDiskLoadStatus = (AtomicCounter*)(textureFileSize + textureCount);
    textureDataBlock = (MemBlock*)(textureDiskLoadStatus + textureCount);
    textureInfo = (PakTextureInfo*)(textureDataBlock + textureCount);
    textureGpuUpload = (u8*)(textureInfo + textureCount);
    textureAge = (i32*)(textureGpuUpload + textureCount);

    i64 fileOffsetsSize = textureCount * sizeof(PakSubFileDesc);
    MemBlock b = MEM_ALLOC(fileOffsetsSize);
    assert(b.ptr);

    PakSubFileDesc* fileOffsets = (PakSubFileDesc*)b.ptr;
    fileReadAdvance(&fileTexture, fileOffsetsSize, fileOffsets);

    for(i32 i = 0; i < textureCount; ++i) {
        textureFileOffset[i] = fileOffsets[i].offset;
        textureFileSize[i] = fileOffsets[i].size;
        //LOG("%d offset=%d", i, textureOffset[i]);
    }

    MEM_DEALLOC(b);

    if(!loadTileTextureIds()) {
        return false;
    }

    if(!loadSectorKeyx()) {
        return false;
    }

    if(!loadFloorData()) {
        return false;
    }

    if(!pak_mixedRead(&mixed)) {
        return false;
    }

    if(!pak_staticRead("../sacred_data/Static.pak", &statics)) {
        return false;
    }

    if(!pak_itemRead("../sacred_data/items.pak", &itemTypes)) {
        return false;
    }

    LOG_SUCC("Resource> initialized");

    return true;
}

bool loadTileTextureIds()
{
    FileBuffer fb = fileReadWhole("../sacred_data/tiles.pak");
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(MEM_DEALLOC(fb.block));

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;


    const i32 tileCount = entryCount / 18 + 1;
    tileTextureIdCount = tileCount;
    tileTextureIdBlock = MEM_ALLOC(sizeof(*tileTextureId) * tileCount);
    assert(tileTextureIdBlock.ptr);
    tileTextureId = (u16*)tileTextureIdBlock.ptr;

    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));
    /*i32 lastTexId = -1;
    i32 streak = 17;*/

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTile* tile = (PakTile*)(top + offset);
        assert(i/18 < tileCount);
        tileTextureId[i/18] = tile->textureId;
        /*if(lastTexId == tile->textureId) {
            streak++;
            assert(streak < 18);
        }
        else {
            assert(streak == 17);
            lastTexId = tile->textureId;
            streak = 0;
            LOG_DBG("tile %d newtexId=%d", i, tile->textureId);
        }*/
    }

    LOG_SUCC("Resource> tiles.pak loaded");
    return true;
}

bool loadSectorKeyx()
{
    FileBuffer fbKeyx = fileReadWhole("../sacred_data/sectors.keyx");
    if(fbKeyx.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbKeyx.block.dealloc());

    const u8* top = (const u8*)fbKeyx.block.ptr;

    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    sectorCount = entryCount;

    LOG("loadSectorKeyx()> header.worldX=%d header.worldY=%d",
        header->worldX, header->worldY);

    sectorInfoBlock = MEM_ALLOC(entryCount * sizeof(*sectorInfo));
    assert(sectorInfoBlock.ptr);
    sectorInfo = (SectorInfo*)sectorInfoBlock.ptr;

    i32 keyxDataOffset = sizeof(PakHeader);
    for(i32 i = 0; i < entryCount; ++i) {
        KeyxSector& ks = *(KeyxSector*)(top + keyxDataOffset);
        keyxDataOffset += sizeof(KeyxSector);

        SectorInfo& si = sectorInfo[i];
        si.sectorId = ks.sectorId;
        si.fileOffset = ks.subs[13].fileOffset;
        si.compressedSize = ks.subs[13].size;
        si.uncompressedSize = ks.subs[15].size;
        si.posX1 = ks.posX1;
        si.posX2 = ks.posX2;
        si.posY1 = ks.posY1;
        si.posY2 = ks.posY2;
        si.numHeights = ks.subs[2].size;

        i32 offsetMapData = ks.subs[11].fileOffset;
        i32 offsetHeightData = ks.subs[12].fileOffset;
        assert(offsetMapData == 32);
        assert(offsetHeightData == 131104);
    }

    LOG_SUCC("Resource> sectors.keyx loaded");

    if(!fileOpenToRead("../sacred_data/sectors.wldx", &fileSectors)) {
        return false;
    }

    sectorDataBlock = MEM_ALLOC(Megabyte(20));
    sectorRingAlloc.init(sectorDataBlock);
    return true;
}

SectorxData* loadSectorData(i32 sectorId)
{
    assert(sectorId >= 0 && sectorId < sectorCount);

    const i32 fileOffset = sectorInfo[sectorId].fileOffset;
    const i32 compSize = sectorInfo[sectorId].compressedSize;
    const i32 uncompSize = sectorInfo[sectorId].uncompressedSize;

    u8* fileBuffer = fileRingAlloc(compSize);
    MemBlock outputBlock = sectorRingAlloc.ALLOC(uncompSize);
    assert(outputBlock.ptr);
    SectorxData* output = (SectorxData*)outputBlock.ptr;

    deflateSectorData(&fileSectors, fileOffset, compSize, uncompSize, fileBuffer, (u8*)output);

#if 0
    char path[256];
    sprintf(path, "sector_data.%d", sectorId);
    fileWriteBuffer(path, (const char*)output, uncompSize);
#endif

    return output;
}

const SectorInfo& getSectorInfo(i32 sectorId)
{
    assert(sectorId >= 0 && sectorId < sectorCount);
    return sectorInfo[sectorId];
}

bool loadFloorData()
{
    FileBuffer fb = fileReadWhole("../sacred_data/Floor.PAK");
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader& header = *(PakHeader*)top;
    floorEntryCount = header.entryCount;
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

    floorData = MEM_ALLOC(sizeof(*floors) * floorEntryCount);
    assert(floorData.ptr);
    floors = (FloorEntry*)floorData.ptr;

    LOG_DBG("Resource> floorCount=%d floorData=%lldmb", floorEntryCount, floorData.size/(1024*1024));

    top += fileDesc[0].offset;
    memmove(floors, top, sizeof(FloorEntry) * floorEntryCount);

    return true;
}

void deinit()
{
    MEM_DEALLOC(textureMetaBlock);
    MEM_DEALLOC(textureFileAndRawBlock);
    MEM_DEALLOC(tempTextureBuffBlock);
    MEM_DEALLOC(tileTextureIdBlock);
    MEM_DEALLOC(sectorInfoBlock);
    MEM_DEALLOC(sectorDataBlock);
    MEM_DEALLOC(floorData);
    MEM_DEALLOC(mixed.block);

    fileClose(&fileTexture);
    gpu.deinit();
}

void newFrame()
{
    gpu.newFrame();
    resetTempAllocator();

    // texture age in frames
    for(i32 i = 0; i < textureCount; ++i) {
        if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::PROCESSED) {
            textureAge[i]++;
        }
    }

    _pakTexIdUpload.clearPOD();
    _dataUpload.clearPOD();
    _texInfoUpload.clearPOD();

    // decompress texture if necessary and upload them to the gpu (if requested)
    i32 workDoneCount = 0;
    for(i32 i = 0; i < textureCount && workDoneCount < FRAME_MAX_PROCESSING_COUNT; ++i) {
        if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::FILE_LOADED) {
            //LOG("Resource> texId=%d file loaded", i);
            i32 width, height, type, textureSize;
            char name[32];
            pak_textureRead((char*)textureDataBlock[i].ptr, textureDataBlock[i].size,
                            &width, &height, &type, tempTextureBuff, &textureSize, name);
            assert(textureSize < TEXTURE_TEMP_SIZE);

            textureDataBlock[i] = textureDataAllocator.ALLOC(textureSize);
            if(!textureDataBlock[i].ptr) {
                // evict old texture data
                textureDataBlock[i] = evictTexture(textureSize);
            }
            memmove(textureDataBlock[i].ptr, tempTextureBuff, textureSize);

            PakTextureInfo& info = textureInfo[i];
            info.width = width;
            info.height = height;
            info.type = (PakTextureType)type;
#ifdef CONF_DEBUG
            memmove(info.name, name, 32);
#endif

            textureDiskLoadStatus[i].decrement(); // -> PROCESSED
            workDoneCount++;
        }

        if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::PROCESSED && textureGpuUpload[i]) {
            _pakTexIdUpload.pushPOD(&i);
            _dataUpload.pushPOD((u8**)&textureDataBlock[i].ptr);
            _texInfoUpload.pushPOD(&textureInfo[i]);
            textureGpuUpload[i] = 0;
        }
    }

    const i32 toUploadCount = _pakTexIdUpload.count();
    if(toUploadCount > 0) {
        gpu.uploadTextures(_pakTexIdUpload.data(), _texInfoUpload.data(), _dataUpload.data(), toUploadCount);
    }
}

u8* fileRingAlloc(i64 size)
{
    if(fileRingCursor + size > FILE_RING_BUFFER_SIZE) {
        fileRingCursor = 0;
        assert(fileRingCursor + size <= FILE_RING_BUFFER_SIZE);
    }

    u8* r = fileData + fileRingCursor;
    fileRingCursor += size;
    return r;
}

MemBlock evictTexture(i64 size)
{
    //LOG_DBG("Resource> evictTexture(%lld)", size);

    for(i32 tries = 0; tries < 100; ++tries) {
        i32 oldestTextureAge = 0;
        i32 oldestTexture = -1;

        for(i32 i = 0; i < textureCount; ++i) {
            if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::PROCESSED &&
               textureAge[i] > oldestTextureAge) {
                oldestTextureAge = textureAge[i];
                oldestTexture = i;
            }
        }

        assert(oldestTexture != -1);

        // delete texture data
        MEM_DEALLOC(textureDataBlock[oldestTexture]);
        textureDiskLoadStatus[oldestTexture].set(0);
        textureGpuUpload[oldestTexture] = 0;
        textureAge[oldestTexture] = 0;

        MemBlock b = textureDataAllocator.ALLOC(size);
        if(b.ptr) {
            //LOG_DBG("Resource> textures evicted = %d", tries+1);
            return b;
        }
    }

    assert(0);
    return NULL_MEMBLOCK;
}

void requestTextures(const i32* pakTextureUIDs, const i32 requestCount)
{
    for(i32 i = 0; i < requestCount; ++i) {
        const i32 texUID = pakTextureUIDs[i];
        assert(texUID > 0 && texUID < textureCount);
        textureAge[texUID] = 0;

        if((LoadStatus)textureDiskLoadStatus[texUID].get() == LoadStatus::NONE &&
           textureDataBlock[texUID].ptr == nullptr) {
            textureDiskLoadStatus[texUID].set((i32)LoadStatus::FILE_LOADING);

            i32 size = textureFileSize[texUID] + sizeof(PakTextureHeader);
            assert(size > 0);
            textureDataBlock[texUID].ptr = fileRingAlloc(size);
            textureDataBlock[texUID].size = size;

            fileAsyncReadAbsolute(&fileTexture, textureFileOffset[texUID], size,
                    (u8*)textureDataBlock[texUID].ptr, &textureDiskLoadStatus[texUID]);
        }
    }
}

void requestGpuTextures(const i32* pakTextureUIDs, u32** out_gpuHandles, const i32 requestCount)
{
    gpu.requestTextures(pakTextureUIDs, out_gpuHandles, requestCount);

    requestTextures(pakTextureUIDs, requestCount);
    for(i32 i = 0; i < requestCount; ++i) {
        textureGpuUpload[pakTextureUIDs[i]] = 1;
    }
}

};





static ResourceManager DR;

bool resource_init()
{
    return DR.init();
}

void resource_deinit()
{
    DR.deinit();
}

void resource_newFrame()
{
    DR.newFrame();
}

void resource_requestTextures(const i32* textureIds, const i32 textureCount)
{
    DR.requestTextures(textureIds, textureCount);
}

void resource_requestGpuTextures(const i32* textureUIDs, u32** out_gpuHandles, const i32 textureCount)
{
    DR.requestGpuTextures(textureUIDs, out_gpuHandles, textureCount);
}

u32 resource_defaultGpuTexture()
{
    return DR.gpu.gpuTexDefault;
}

SectorxData* resource_loadSector(i32 sectorId)
{
    return DR.loadSectorData(sectorId);
}

const SectorInfo& resource_getSectorInfo(i32 sectorId)
{
    return DR.getSectorInfo(sectorId);
}

u16* resource_getTileTextureIds18()
{
    return DR.tileTextureId;
}

i32 resource_getTextureCount()
{
    return DR.textureCount;
}

PakTextureInfo* resource_getTextureInfos()
{
    return DR.textureInfo;
}

FloorEntry* resource_getFloors()
{
    return DR.floors;
}

i32 resource_getFloorCount()
{
    return DR.floorEntryCount;
}

i32 resource_getTileCount18()
{
    return DR.tileTextureIdCount;
}

PakStatic* resource_getStatic()
{
    return DR.statics.data();
}

PakItemType* resource_getItemTypes()
{
    return DR.itemTypes.data();
}

i32 resource_getStaticCount()
{
    return DR.statics.count();
}

i32 resource_getItemTypesCount()
{
    return DR.itemTypes.count();
}

PakMixedDesc* resource_getMixedDescs()
{
    return DR.mixed.desc;
}

i32 resource_getMixedDescsCount()
{
    return DR.mixed.descCount;
}

PakMixedData* resource_getMixedData()
{
    return DR.mixed.mixed;
}

void resource_getWorldOrigin(i32* x, i32* y)
{
    *x = DR.worldOriginX;
    *y = DR.worldOriginY;
}
