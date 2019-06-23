#include "rs_resources.h"
#include "rs_renderer.h"
#include "rs_file.h"
#include "rs_game.h"
#include "rs_jobs.h"

#include <bgfx/bgfx.h>

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

// TODO: SOA this later on maybe?
struct GpuSectorMesh
{
    i32 sectorId;
    GLuint gpuBuffer;
    i32 vertFloorOff;
    i32 vertMixedOff;
    mat4 matModelTiles;
    mat4 matModelMixed;
};

struct GpuResources
{
bgfx::TextureHandle gpuTexDefault;
bgfx::TextureHandle texGpuId[MAX_GPU_TEXTURES];
i32 texDiskId[MAX_GPU_TEXTURES];
i32 texFramesNotRequested[MAX_GPU_TEXTURES];
AtomicCounter texLoaded[MAX_GPU_TEXTURES];
u8 texSlotOccupied[MAX_GPU_TEXTURES];
RendererFrameData* frameData;
Array<GpuSectorMesh,16> sectorMeshList;

bool init()
{
    memset(texFramesNotRequested, 0, sizeof(texFramesNotRequested));
    memset(texSlotOccupied, 0, sizeof(texSlotOccupied));
    memset(texLoaded, 0, sizeof(texLoaded));

	for(int t = 0; t < MAX_GPU_TEXTURES; t++) {
		texGpuId[t] = BGFX_INVALID_HANDLE;
	}

	const i32 texWidth = 16;
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

	/*TextureDesc2D desc;
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
	frameData->_addTextureCreate(desc, &gpuTexDefault, textureData, sizeof(textureData));*/

	gpuTexDefault = bgfx::createTexture2D(texWidth, texWidth, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_UVW_MIRROR|BGFX_SAMPLER_POINT, bgfx::makeRef((u8*)textureData, texWidth*texWidth*4));

    return true;
}

void destroyTexture(i32 texSlot)
{
	if(texGpuId[texSlot].idx != gpuTexDefault.idx) {
		//frameData->_addTextureToDestroyList(texGpuId[texSlot]);
		bgfx::destroy(texGpuId[texSlot]);
    }
	texGpuId[texSlot] = gpuTexDefault;
    texDiskId[texSlot] = 0;
	texLoaded[texSlot].reset();
	texSlotOccupied[texSlot] = false;
}

void newFrame()
{
	ProfileFunction();

    // find unused textures and destroy them
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i]) {
            texFramesNotRequested[i]++;

            if(texFramesNotRequested[i] > 10000) {
                destroyTexture(i);
                //LOG_DBG("ResourceGpu> evicting texture %d", i);
            }
        }
    }
}

i32 _occupyNextTextureSlot(i32 pakTexId)
{
    const i32 fastId = pakTexId % MAX_GPU_TEXTURES;
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        const i32 id = (fastId + i) % MAX_GPU_TEXTURES;
        if(texSlotOccupied[id] == false) {
            texSlotOccupied[id] = true;
            return id;
        }
    }

    i32 oldestFrameCount = 0;
    i32 oldestId = -1;
    for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
        if(texSlotOccupied[i] && texFramesNotRequested[i] > oldestFrameCount &&
		   texLoaded[i].get() == 1) {
            oldestId = i;
            oldestFrameCount = texFramesNotRequested[i];
        }
    }

	/*assert(oldestId >= 0);
	assert(oldestFrameCount > 0);*/

	if(oldestId == -1) {
		LOG_DBG("ResourceGpu> Warning, requesting a lot of textures quickly");
		oldestId = fastId;
	}

    destroyTexture(oldestId);
    texSlotOccupied[oldestId] = true;
    return oldestId;
}

void findTexturesOrSlotThem(const i32* inPakTextureIds, bgfx::TextureHandle* outGpuTexHandles, u8* out_pFound, const i32 requestCount)
{
    // assign already loaded/loading textures
    for(i32 r = 0; r < requestCount; ++r) {
        const i32 pakTexId = inPakTextureIds[r];

        bool found = false;

        const i32 fastId = pakTexId % MAX_GPU_TEXTURES;
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            const i32 id = (fastId + i) % MAX_GPU_TEXTURES;
            if(texSlotOccupied[id] && texDiskId[id] == pakTexId) {
				outGpuTexHandles[r] = texGpuId[id];
                texFramesNotRequested[id] = 1;
                found = true;
                break;
            }
        }

        if(!found) {
            const i32 newId = _occupyNextTextureSlot(pakTexId);
            texGpuId[newId] = gpuTexDefault;
			outGpuTexHandles[r] = texGpuId[newId];
            texDiskId[newId] = pakTexId;
            texFramesNotRequested[newId] = 1;
            // TODO: it increments here but decrements in ResourceManager?
            // choose one
			texLoaded[newId].reset();
        }

		out_pFound[r] = found;
	}
}

void uploadTextures(i32* pakTextureUIDs, PakTextureInfo* textureInfos, u8** textureData, const i32 count)
{
	// for all remaining requests create a new gpu texture and assign it
    for(i32 r = 0; r < count; ++r) {
        const i32 pakTexId = pakTextureUIDs[r];
        i32 gpuId = -1;

        const i32 fastId = pakTexId % MAX_GPU_TEXTURES;
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            const i32 id = (fastId + i) % MAX_GPU_TEXTURES;
            if(texSlotOccupied[id] && texDiskId[id] == pakTexId) {
                gpuId = id;
                break;
            }
        }

        if(gpuId == -1 || texLoaded[gpuId].get()) {
            continue;
        }

        //LOG_DBG("ResourcesGpu> uploading pakId=%d gpuId=%d", pakTexId, gpuId);

		bgfx::TextureFormat::Enum texFormat;
		u16 texWidth = textureInfos[r].width;
		u16 texHeight = textureInfos[r].height;

        u32 textureDataSize = 0;
        if(textureInfos[r].type == PakTextureType::TYPE_RGBA8) {
			texFormat = bgfx::TextureFormat::RGBA8;
            textureDataSize = textureInfos[r].width * textureInfos[r].height * 4;
        }
        else {
			texFormat = bgfx::TextureFormat::RGBA4;
            textureDataSize = textureInfos[r].width * textureInfos[r].height * 2;
        }

        texLoaded[gpuId].increment();
		//frameData->_addTextureCreate(desc, &texGpuId[gpuId], textureData[r], textureDataSize);
		texGpuId[gpuId] = bgfx::createTexture2D(texWidth, texHeight, false, 1, texFormat, BGFX_SAMPLER_POINT, bgfx::makeRef(textureData[r], textureDataSize));
	}
}

void shutdown()
{
	bgfx::destroy(gpuTexDefault);
	for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
		if(bgfx::isValid(texGpuId[i]))
			bgfx::destroy(texGpuId[i]);
	}
}

void debugUi()
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
			texGpuId[i].idx, texDiskId[i], texFramesNotRequested[i]);
    }
    ImGui::End();

    ImGui::End();
}
};

struct ThreadTempStorage
{
	AllocatorStack allocator;

	ThreadTempStorage()
	{
		allocator.init(MEM_MALLOCATOR.ALLOC(Megabyte(10)));
		allocator.setNoFallback();
		LOG("ThreadTempStorage thid=%x", threadGetId());
	}

	~ThreadTempStorage()
	{
		allocator.release();
	}
};

ThreadTempStorage& getThreadTempStorage()
{
	thread_local ThreadTempStorage tempStorage;
	return tempStorage;
}

#define TEXTURE_CACHE_SIZE Megabyte(200)
#define FRAME_MAX_PROCESSING_COUNT 24

/*
 * Resource manager
 *
 * Texture request process:
 * - if not loaded, request file read
 */

struct ResourceManager {

DiskFile fileTexturePak;
i32 textureCount;
i32* textureFileOffset;
i32* textureFileSize;
AtomicCounter* textureDiskLoadStatus; // 2 == loading from disk, 1 == loaded
MemBlock* textureFileCache;
PakTextureInfo* textureInfo;
u8* textureGpuUpload;
i32* textureAge; // in frames
MemBlock textureMetaBlock;

AllocatorBucket textureFileCacheAllocator;
Mutex textureFileCacheMutex;

GpuResources gpu;

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

MemBlock floorData;
FloorEntry* floors;
//const i32 floorCount = 10000;
i32 floorEntryCount;

Array<PakStatic> statics;
Array<PakItemType> itemTypes;

PakMixedFileData mixed;

enum class LoadStatus: i32 {
	NONE = 0,
	FILE_LOADED,
	FILE_LOADING,
};

AtomicCounter jobsSuccessfullCount;

bool init()
{
	ProfileFunction();

    gpu.init();

	textureFileCacheAllocator.init(MEM_MALLOCATOR.ALLOC(TEXTURE_CACHE_SIZE), 2048);
	textureFileCacheAllocator.setNoFallback();

	auto t0 = timeNow();

#if 0
	if(!loadTexturePak()) {
		return false;
	}

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
#else
	AtomicCounter finished;
	jobsSuccessfullCount.reset();

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(rThis.loadTexturePak()) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(rThis.loadTileTextureIds()) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(rThis.loadSectorKeyx()) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(rThis.loadFloorData()) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(pak_mixedRead(&rThis.mixed)) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(pak_staticRead("../sacred_data/Static.pak", &rThis.statics)) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobRun(this, [](void* data) {
		ResourceManager& rThis = *(ResourceManager*)data;
		if(pak_itemRead("../sacred_data/items.pak", &rThis.itemTypes)) {
			rThis.jobsSuccessfullCount.increment();
		}
	}, &finished);

	jobWait(&finished);

	if(jobsSuccessfullCount.get() != 7) {
		return false;
	}
#endif

	LOG_SUCC("Resource> initialized (%.5fs)", timeDurSince(t0));
    return true;
}

bool loadTexturePak()
{
	ProfileFunction();

	if(!fileOpenToRead("../sacred_data/texture.pak", &fileTexturePak)) {
		return false;
	}

	// Setup texture related stuff
	PakHeader header;
	fileReadAdvance(&fileTexturePak, sizeof(header), &header);
	textureCount = header.entryCount;

	// alloc texture metadata
	textureMetaBlock = MEM_ALLOC((sizeof(*textureFileOffset) + sizeof(*textureFileSize) +
			sizeof(*textureDiskLoadStatus) +
			sizeof(*textureFileCache) + sizeof(*textureInfo) + sizeof(*textureGpuUpload) +
			sizeof(*textureAge)) * textureCount);
	assert(textureMetaBlock.isValid());

	LOG_DBG("Resource> textureMetaBlock size=%lldkb", textureMetaBlock.size/1024);

	textureFileOffset = (i32*)textureMetaBlock.ptr;
	textureFileSize = (i32*)(textureFileOffset + textureCount);
	textureDiskLoadStatus = (AtomicCounter*)(textureFileSize + textureCount);
	textureFileCache = (MemBlock*)(textureDiskLoadStatus + textureCount);
	textureInfo = (PakTextureInfo*)(textureFileCache + textureCount);
	textureGpuUpload = (u8*)(textureInfo + textureCount);
	textureAge = (i32*)(textureGpuUpload + textureCount);

	i64 fileOffsetsSize = textureCount * sizeof(PakSubFileDesc);
	MemBlock b = MEM_ALLOC(fileOffsetsSize);
	assert(b.isValid());

	PakSubFileDesc* fileOffsets = (PakSubFileDesc*)b.ptr;
	fileReadAdvance(&fileTexturePak, fileOffsetsSize, fileOffsets);

	for(i32 i = 0; i < textureCount; ++i) {
		textureFileOffset[i] = fileOffsets[i].offset;
		textureFileSize[i] = fileOffsets[i].size;
		//LOG("%d offset=%d", i, textureOffset[i]);
	}

	MEM_DEALLOC(b);
	return true;
}

bool loadTileTextureIds()
{
	ProfileFunction();

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

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTile* tile = (PakTile*)(top + offset);
        assert(i/18 < tileCount);
        tileTextureId[i/18] = tile->textureId;
    }

    LOG_SUCC("Resource> tiles.pak loaded");
    return true;
}

bool loadSectorKeyx()
{
	ProfileFunction();

    FileBuffer fbKeyx = fileReadWhole("../sacred_data/sectors.keyx");
    if(fbKeyx.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbKeyx.block.dealloc());

    const u8* top = (const u8*)fbKeyx.block.ptr;

    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;

    // first pass to determine sector count
    i32 keyxDataOffset = sizeof(PakHeader);
    sectorCount = 0;
    for(i32 i = 0; i < entryCount; ++i) {
        KeyxSector& ks = *(KeyxSector*)(top + keyxDataOffset);
        keyxDataOffset += sizeof(KeyxSector);
		sectorCount = MAX(sectorCount, ks.sectorId+1);
    }

    assert(sectorCount == 6051);

    LOG("loadSectorKeyx()> header.worldX=%d header.worldY=%d",
        header->worldX, header->worldY);

    sectorInfoBlock = MEM_ALLOC(sectorCount * sizeof(SectorInfo));
    assert(sectorInfoBlock.ptr);
    sectorInfo = (SectorInfo*)sectorInfoBlock.ptr;

    keyxDataOffset = sizeof(PakHeader);
    u64 sectorAllDataSize = 0;
    for(i32 i = 0; i < entryCount; ++i) {
        KeyxSector& ks = *(KeyxSector*)(top + keyxDataOffset);
        keyxDataOffset += sizeof(KeyxSector);

        SectorInfo& si = sectorInfo[ks.sectorId];
        si.sectorId = ks.sectorId;
        assert(si.sectorId >= 0 && si.sectorId < 6051);
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
        assert(si.compressedSize > 0 && si.uncompressedSize > 0);
        sectorAllDataSize += si.uncompressedSize;
    }

    LOG_DBG("Resource> sectorAllDataSize: %lld ko", sectorAllDataSize/1024);
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
	ProfileFunction();
	// FIXME: reenable
	/*
    assert(sectorId > 0 && sectorId < sectorCount);

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
	*/
	return nullptr;
}

const SectorInfo& getSectorInfo(i32 sectorId)
{
    assert(sectorId >= 0 && sectorId < sectorCount);
    return sectorInfo[sectorId];
}

bool loadFloorData()
{
	ProfileFunction();

    FileBuffer fb = fileReadWhole("../sacred_data/Floor.PAK");
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
	defer(MEM_DEALLOC(fb.block));

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

void shutdown()
{
	textureFileCacheAllocator.release();
    MEM_DEALLOC(textureMetaBlock);
    MEM_DEALLOC(tileTextureIdBlock);
    MEM_DEALLOC(sectorInfoBlock);
    MEM_DEALLOC(sectorDataBlock);
    MEM_DEALLOC(floorData);
    MEM_DEALLOC(mixed.block);

	fileClose(&fileTexturePak);
	gpu.shutdown();
}

void newFrame()
{
	ProfileFunction();

    gpu.newFrame();
    resetTempAllocator();

    // texture age in frames
    for(i32 i = 0; i < textureCount; ++i) {
		if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::FILE_LOADED) {
            textureAge[i]++;
        }
    }

	static Array<i32,64> pakTexIdUpload;
	static Array<u8*,64> dataUpload;
	static Array<PakTextureInfo,64> texInfoUpload;
	pakTexIdUpload.clearPOD();
	dataUpload.clearPOD();
	texInfoUpload.clearPOD();

	// upload to the gpu (if requested)
	i32 toUploadMax = 4;
	for(i32 i = 0; i < textureCount && toUploadMax > 0; ++i) {
		if(textureGpuUpload[i] && (LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::FILE_LOADED) {
			pakTexIdUpload.pushPOD(&i);
			dataUpload.pushPOD((u8**)&textureFileCache[i].ptr);
			texInfoUpload.pushPOD(&textureInfo[i]);
            textureGpuUpload[i] = 0;
			toUploadMax--;
        }
    }

	const i32 toUploadCount = pakTexIdUpload.count();
    if(toUploadCount > 0) {
		//LOG("ReourceManager> uploading textures to gpu: %d", toUploadCount);
		gpu.uploadTextures(pakTexIdUpload.data(), texInfoUpload.data(), dataUpload.data(), toUploadCount);
    }
}

MemBlock evictTextureFileCacheAndAllocNewOne(i64 size)
{
	ProfileFunction();
    //LOG_DBG("Resource> evictTexture(%lld)", size);

    for(i32 tries = 0; tries < 100; ++tries) {
        i32 oldestTextureAge = 0;
        i32 oldestTexture = -1;

        for(i32 i = 0; i < textureCount; ++i) {
			if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::FILE_LOADED &&
			   textureAge[i] > oldestTextureAge) {
				assert(textureFileCache[i].isValid());
                oldestTextureAge = textureAge[i];
                oldestTexture = i;
            }
        }

        assert(oldestTexture != -1);

        // delete texture data
		MEM_DEALLOC(textureFileCache[oldestTexture]);
		textureDiskLoadStatus[oldestTexture].reset();
        textureGpuUpload[oldestTexture] = 0;
        textureAge[oldestTexture] = 0;

        // try to alloc new texture
		MemBlock b = textureFileCacheAllocator.ALLOC(size);
		if(b.isValid()) {
			//LOG_DBG("Resource> textures evicted = %d", tries+1);
			return b;
		}
    }

	LOG_ERR("ERROR: could not evict oldest texture, cache seems too small");
	assert_msg(0, "ERROR: could not evict oldest texture, cache seems too small");
	return NULL_MEMBLOCK;
}

void requestTextureFileLoad(const i32* pakTextureUIDs, u8* pSkip, const i32 requestCount)
{
	for(i32 i = 0; i < requestCount; ++i) {
        const i32 texUID = pakTextureUIDs[i];
        assert(texUID > 0 && texUID < textureCount);
        textureAge[texUID] = 0;

		if(pSkip && pSkip[i]) {
			continue;
		}

        if((LoadStatus)textureDiskLoadStatus[texUID].get() == LoadStatus::NONE &&
		   !textureFileCache[texUID].isValid()) {
			textureDiskLoadStatus[texUID].set((i32)LoadStatus::FILE_LOADING);

			i32 fileSize = textureFileSize[texUID] + sizeof(PakTextureHeader);
			assert(fileSize > 0);

			struct TextureFileLoadAndProcessJob {
				const DiskFile* file;
				i64 start;
				i64 fileBufferSize;
				AtomicCounter* pCounter;
				ResourceManager* pResMngr;
				i32 texUID;
			};

			TextureFileLoadAndProcessJob jd = {
				&fileTexturePak,
				textureFileOffset[texUID],
				fileSize,
				&textureDiskLoadStatus[texUID],
				this,
				texUID
			};

			jobRunEx(&jd, sizeof(jd), [](void* pData) {
				TextureFileLoadAndProcessJob data = *(TextureFileLoadAndProcessJob*)pData;
				ResourceManager& rThis = *data.pResMngr;
				const i32 texUID = data.texUID;

				auto& ts = getThreadTempStorage();
				MemBlock extractedBlock = ts.allocator.ALLOC(Megabyte(5));
				assert(extractedBlock.isValid());

				MemBlock fileBuffer = ts.allocator.ALLOC(data.fileBufferSize);
				assert(fileBuffer.isValid());

				fileReadFromPos(data.file, data.start, data.fileBufferSize, (u8*)fileBuffer.ptr);

				i32 width, height, type, textureSize;
				char name[32];

				pak_textureRead((char*)fileBuffer.ptr, data.fileBufferSize, &width, &height, &type, (u8*)extractedBlock.ptr, &textureSize, name);
				assert(textureSize < extractedBlock.size);

				//LOG("thid=%x width=%d height=%d type=%d size=%d name=%s", threadGetId(), width, height, type, textureSize, name);

				rThis.textureFileCacheMutex.lock();

				rThis.textureFileCache[texUID] = rThis.textureFileCacheAllocator.ALLOC(textureSize);
				if(!rThis.textureFileCache[texUID].isValid()) {
					//assert(0); // possible data race here, fix
					// evict old texture data
					rThis.textureFileCache[texUID] = rThis.evictTextureFileCacheAndAllocNewOne(textureSize);
				}

				rThis.textureFileCacheMutex.unlock();

				memmove(rThis.textureFileCache[texUID].ptr, extractedBlock.ptr, textureSize);

				ts.allocator.deallocTo(0); // empty allocator

				PakTextureInfo& info = rThis.textureInfo[texUID];
				info.width = width;
				info.height = height;
				info.type = (PakTextureType)type;
				#ifdef CONF_DEBUG
					memmove(info.name, name, 32);
				#endif

				data.pCounter->decrement(); // LoadStatus::FILE_LOADED
			}, nullptr);
        }
	}
}

void requestGpuTextures(const i32* pakTextureUIDs, bgfx::TextureHandle* out_gpuHandles, const i32 requestCount)
{
	MemBlock foundBlock = MEM_CONTEXT.tempAllocator->ALLOC(requestCount * sizeof(u8));
	assert(foundBlock.isValid());
	u8* found = (u8*)foundBlock.ptr;

	gpu.findTexturesOrSlotThem(pakTextureUIDs, out_gpuHandles, found, requestCount);
	requestTextureFileLoad(pakTextureUIDs, found, requestCount);

    for(i32 i = 0; i < requestCount; ++i) {
		if(!found[i]) {
			textureGpuUpload[pakTextureUIDs[i]] = 1;
		}
    }

	MEM_DEALLOC(foundBlock);
}

void debugUi()
{
	//gpu.debugUi();

    ImGui::Begin("Resources memory");

	//::Image(gpu.gpuTexDefault, ImVec2(256, 256));

    u64 allocatedSpace, freeSpace;
	textureFileCacheAllocator.getFillInfo(&allocatedSpace, &freeSpace);

    ImGui::Text("Texture allocator");
    ImGui::Text("(%lld Ko / %lld Ko)", allocatedSpace/1024, (allocatedSpace+freeSpace)/1024);
    ImGui::ProgressBar((f32)allocatedSpace/(allocatedSpace+freeSpace));

	/*ImGui::BeginChild("cache_texture_list");
	for(i32 i = 0; i < textureCount; ++i) {
		ImVec4 textColor(1, 1, 1, 1);
		ImGui::TextColored(textColor, "[%4d] load=%d age=%d", i,
			(LoadStatus)textureDiskLoadStatus[i].get(), textureAge[i]);
	}
	ImGui::End();*/

    ImGui::End();
}

};





static ResourceManager s_ResourceManager;

bool resource_init()
{
	return s_ResourceManager.init();
}

void resource_shutdown()
{
	s_ResourceManager.shutdown();
}

void resource_newFrame()
{
	s_ResourceManager.newFrame();
}

void resource_requestTextures(const i32* textureIds, const i32 textureCount)
{
	s_ResourceManager.requestTextureFileLoad(textureIds, nullptr, textureCount);
}

void resource_requestGpuTextures(const i32* textureUIDs, bgfx::TextureHandle* out_gpuHandles, const i32 textureCount)
{
	s_ResourceManager.requestGpuTextures(textureUIDs, out_gpuHandles, textureCount);
}

bgfx::TextureHandle resource_defaultGpuTexture()
{
	return s_ResourceManager.gpu.gpuTexDefault;
}

SectorxData* resource_loadSector(i32 sectorId)
{
	return s_ResourceManager.loadSectorData(sectorId);
}

const SectorInfo& resource_getSectorInfo(i32 sectorId)
{
	return s_ResourceManager.getSectorInfo(sectorId);
}

const SectorInfo* resource_getSectorInfoList()
{
	return s_ResourceManager.sectorInfo;
}

i32 resource_getSectorCount()
{
	return s_ResourceManager.sectorCount;
}

u16* resource_getTileTextureIds18()
{
	return s_ResourceManager.tileTextureId;
}

i32 resource_getTextureCount()
{
	return s_ResourceManager.textureCount;
}

PakTextureInfo* resource_getTextureInfos()
{
	return s_ResourceManager.textureInfo;
}

FloorEntry* resource_getFloors()
{
	return s_ResourceManager.floors;
}

i32 resource_getFloorCount()
{
	return s_ResourceManager.floorEntryCount;
}

i32 resource_getTileCount18()
{
	return s_ResourceManager.tileTextureIdCount;
}

PakStatic* resource_getStatic()
{
	return s_ResourceManager.statics.data();
}

PakItemType* resource_getItemTypes()
{
	return s_ResourceManager.itemTypes.data();
}

i32 resource_getStaticCount()
{
	return s_ResourceManager.statics.count();
}

i32 resource_getItemTypesCount()
{
	return s_ResourceManager.itemTypes.count();
}

PakMixedDesc* resource_getMixedDescs()
{
	return s_ResourceManager.mixed.desc;
}

i32 resource_getMixedDescsCount()
{
	return s_ResourceManager.mixed.descCount;
}

PakMixedData* resource_getMixedData()
{
	return s_ResourceManager.mixed.mixed;
}

void resource_getWorldOrigin(i32* x, i32* y)
{
	*x = s_ResourceManager.worldOriginX;
	*y = s_ResourceManager.worldOriginY;
}

void resources_debugUi()
{
	s_ResourceManager.debugUi();
}
