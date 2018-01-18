#include "rs_resources.h"
#include "rs_renderer.h"
#include "rs_file.h"

struct PakTextureInfo
{
    u16 width;
    u16 height;
    PakTextureType type;
};


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


#define MAX_GPU_TEXTURES 1024

struct GPUResources
{
    u32 gpuTexDefault;
    u32 texGpuId[MAX_GPU_TEXTURES];
    i32 texDiskId[MAX_GPU_TEXTURES];
    i32 texFramesNotRequested[MAX_GPU_TEXTURES];
    AtomicCounter texLoaded[MAX_GPU_TEXTURES];
    u8 texSlotOccupied[MAX_GPU_TEXTURES];
    TextureDesc2D texDesc[MAX_GPU_TEXTURES];

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

        static TextureDesc2D desc;
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

        CommandList list;
        list.createTexture2D(&desc, &gpuTexDefault);
        renderer_pushCommandList(list);
        return true;
    }

    void newFrame()
    {
        CommandList cmds;

        // find unused textures and destroy them
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i]) {
                texFramesNotRequested[i]++;

                if(texFramesNotRequested[i] > 10) {
                    cmds.destroyTexture(texGpuId[i]);
                    texGpuId[i] = gpuTexDefault;
                    texDiskId[i] = 0;
                    texSlotOccupied[i] = false;
                }
            }
        }

        renderer_pushCommandList(cmds);
    }

    i32 _occupyNextTextureSlot()
    {
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i] == false) {
                texSlotOccupied[i] = true;
                return i;
            }
        }

        LOG_WARN("GPURes> Warning, requesting a lot of textures quickly");

        i32 oldestFrameCount = 0;
        i32 oldestId = -1;
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i] && texFramesNotRequested[i] > oldestFrameCount &&
               texLoaded[i].get() == 1) {
                oldestId = i;
                oldestFrameCount = texFramesNotRequested[i];
            }
        }

        assert(oldestFrameCount > 0);
        assert(oldestId >= 0);

        CommandList cmds;
        cmds.destroyTexture(texGpuId[oldestId]);
        renderer_pushCommandList(cmds);
        texGpuId[oldestId] = gpuTexDefault;
        texDiskId[oldestId] = 0;
        texLoaded[oldestId]._count = 0;
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
                    texFramesNotRequested[i] = 0;
                    found = true;
                    break;
                }
            }

            if(!found) {
                i32 newId = _occupyNextTextureSlot();
                outGpuTexHandles[r] = &texGpuId[newId];
                texGpuId[newId] = gpuTexDefault;
                texDiskId[newId] = pakTexId;
                texFramesNotRequested[newId] = 0;
                // TODO: it increments here but decrements in ResourceManager?
                // choose one
                texLoaded[newId].set(0);
            }
        }
    }

    void uploadTextures(i32* pakTextureUIDs, PakTextureInfo* textureInfos, u8** textureData, const i32 count)
    {
        // for all remaining requests create a new gpu texture and assign it
        CommandList cmds;
        for(i32 r = 0; r < count; ++r) {
            const i32 pakTexId = pakTextureUIDs[r];
            i32 gpuId = -1;

            for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
                if(texSlotOccupied[i] && texDiskId[i] == pakTexId) {
                    gpuId = i;
                    break;
                }
            }

            assert(gpuId != -1);

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


            cmds.createTexture2D(&desc, &texGpuId[gpuId]);
            cmds.counterIncrement(&texLoaded[gpuId]);
        }
        renderer_pushCommandList(cmds);
    }

    void deinit()
    {
        CommandList cmds;
        for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
            if(texSlotOccupied[i]) {
                cmds.destroyTexture(texGpuId[i]);
            }
        }
        cmds.execute();
        renderer_pushCommandList(cmds);
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

#define FILE_DATA_SIZE Megabyte(100)
#define TEXTURE_DATA_SIZE Megabyte(200)
#define TEXTURE_TEMP_SIZE Megabyte(5)

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
AtomicCounter* textureDiskLoadStatus; // 2 == loading from disk, 1 == loaded
MemBlock* textureDataBlock;
PakTextureInfo* textureInfo;
u8* textureGpuUpload;
MemBlock textureMetaBlock;

u8* fileData;
u8* textureRawData;
MemBlock textureFileAndRawBlock;
intptr_t fileRingCursor = 0;
AllocatorBucket textureDataAllocator;

u8* tempTextureBuff;
MemBlock tempTextureBuffBlock;

GPUResources gpu;

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

    textureFileAndRawBlock = MEM_ALLOC(TEXTURE_DATA_SIZE + FILE_DATA_SIZE);
    assert(textureFileAndRawBlock.ptr);
    textureRawData = (u8*)textureFileAndRawBlock.ptr;
    fileData = textureRawData + TEXTURE_DATA_SIZE;

    LOG_DBG("Resource> textureFileAndRawBlock size=%lldmb", (TEXTURE_DATA_SIZE + FILE_DATA_SIZE)/(1024*1024));

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
    textureMetaBlock = MEM_ALLOC((sizeof(i32) + sizeof(AtomicCounter) + sizeof(MemBlock) +
                                  sizeof(PakTextureInfo) + sizeof(u8)) * textureCount);
    assert(textureMetaBlock.ptr);

    LOG_DBG("Resource> textureMetaBlock size=%lldkb", textureMetaBlock.size/1024);

    textureFileOffset = (i32*)textureMetaBlock.ptr;
    textureDiskLoadStatus = (AtomicCounter*)(textureFileOffset + textureCount);
    textureDataBlock = (MemBlock*)(textureDiskLoadStatus + textureCount);
    textureInfo = (PakTextureInfo*)(textureDataBlock + textureCount);
    textureGpuUpload = (u8*)(textureInfo + textureCount);

    i64 fileOffsetsSize = textureCount * sizeof(PakSubFileDesc);
    MemBlock b = MEM_ALLOC(fileOffsetsSize);
    assert(b.ptr);

    PakSubFileDesc* fileOffsets = (PakSubFileDesc*)b.ptr;
    fileReadAdvance(&fileTexture, fileOffsetsSize, fileOffsets);

    for(i32 i = 0; i < textureCount; ++i) {
        textureFileOffset[i] = fileOffsets[i].offset;
        //LOG("%d offset=%d", i, textureOffset[i]);
    }

    MEM_DEALLOC(b);

    return true;
}

void deinit()
{
    MEM_DEALLOC(textureMetaBlock);
    MEM_DEALLOC(textureFileAndRawBlock);
    MEM_DEALLOC(tempTextureBuffBlock);
    fileClose(&fileTexture);
    gpu.deinit();
}

void newFrame()
{
    resetTempAllocator();

    Array<i32,256> pakTexIdUpload;
    Array<u8*,256> dataUpload;
    Array<PakTextureInfo,256> texInfoUpload;
    pakTexIdUpload.allocator = MEM_CONTEXT.tempAllocator;
    dataUpload.allocator = MEM_CONTEXT.tempAllocator;
    texInfoUpload.allocator = MEM_CONTEXT.tempAllocator;

    // decompress texture if necessary and upload them to the gpu (if requested)
    for(i32 i = 0; i < textureCount; ++i) {
        if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::FILE_LOADED) {
            //LOG("Resource> texId=%d file loaded", i);
            i32 width, height, type, textureSize;
            pak_textureRead((char*)textureDataBlock[i].ptr, textureDataBlock[i].size,
                            &width, &height, &type, tempTextureBuff, &textureSize);
            assert(textureSize < TEXTURE_TEMP_SIZE);

            textureDataBlock[i] = textureDataAllocator.ALLOC(textureSize);
            if(!textureDataBlock[i].ptr) {
                // evict old texture data
                assert(0); // TODO: implement
            }
            memmove(textureDataBlock[i].ptr, tempTextureBuff, textureSize);

            textureDiskLoadStatus[i].decrement();

            PakTextureInfo& info = textureInfo[i];
            info.width = width;
            info.height = height;
            info.type = (PakTextureType)type;

            pakTexIdUpload.pushPOD(&i);
            dataUpload.pushPOD((u8**)&textureDataBlock[i].ptr);
            texInfoUpload.pushPOD(&info);
        }
    }

    const i32 toUploadCount = pakTexIdUpload.count();
    if(toUploadCount > 0) {
        gpu.uploadTextures(pakTexIdUpload.data(), texInfoUpload.data(), dataUpload.data(), toUploadCount);
    }
}

u8* fileRingAlloc(i64 size)
{
    if(fileRingCursor + size > FILE_DATA_SIZE) {
        fileRingCursor = 0;
        assert(fileRingCursor + size <= FILE_DATA_SIZE);
    }

    u8* r = fileData + fileRingCursor;
    fileRingCursor += size;
    return r;
}

void requestTextures(const i32* textureIds, const i32 requestCount)
{
    /*for(i32 i = 0; i < requestCount; ++i) {
        fileAsyncReadAbsolute(&fileTexture, 0, 0, [](char* buff, i64 size, MemBlock block) {
            resource_loadTexture(buff, size);
            MEM_DEALLOC(block);
        });
    }*/
}

void requestGpuTextures(const i32* textureUIDs, u32** out_gpuHandles, const i32 requestCount)
{
    gpu.requestTextures(textureUIDs, out_gpuHandles, requestCount);

    for(i32 i = 0; i < requestCount; ++i) {
        const i32 texUID = textureUIDs[i];
        assert(texUID > 0 && texUID < textureCount);
        //out_gpuHandles[i] = &gpu.gpuTexDefault;

        if((LoadStatus)textureDiskLoadStatus[texUID].get() == LoadStatus::NONE &&
           textureDataBlock[texUID].ptr == nullptr) {
            textureDiskLoadStatus[texUID].set((i32)LoadStatus::FILE_LOADING);

            i32 size = textureFileOffset[textureUIDs[i] + 1] - textureFileOffset[textureUIDs[i]];
            assert(size > 0);
            textureDataBlock[texUID].ptr = fileRingAlloc(size);
            textureDataBlock[texUID].size = size;

            fileAsyncReadAbsolute(&fileTexture, textureFileOffset[textureUIDs[i]], size,
                    (u8*)textureDataBlock[texUID].ptr, &textureDiskLoadStatus[texUID]);
        }
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
