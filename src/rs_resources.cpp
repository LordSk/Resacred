#include "rs_resources.h"
#include "rs_renderer.h"
#include "rs_file.h"

static void resource_loadTexture(char* buff, i64 size);

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

    void requestTextures(i32* inPakTextureIds, u32** outGpuTexHandles, const i32 requestCount)
    {
        /*memset(outGpuTexHandles, 0, sizeof(outGpuTexHandles[0]) * requestCount);

        // assign already loaded/loading textures
        for(i32 r = 0; r < requestCount; ++r) {
            const i32 pakTexId = inPakTextureIds[r] ;
            assert(pakTexId >= 0 && pakTexId < diskTextures->textureCount);

            for(i32 i = 0; i < MAX_GPU_TEXTURES; ++i) {
                if(texSlotOccupied[i] && texDiskId[i] == pakTexId) {
                    outGpuTexHandles[r] = &texGpuId[i];
                    texFramesNotRequested[i] = 0;
                    break;
                }
            }
        }

        // for all remaining requests create a new gpu texture and assign it
        CommandList cmds;
        for(i32 r = 0; r < requestCount; ++r) {
            if(outGpuTexHandles[r] != 0) {
                continue;
            }

            // FIXME: can somehow bypass first assert??
            const i32 reqPakTexId = inPakTextureIds[r];
            assert(reqPakTexId >= 1 && reqPakTexId < diskTextures->textureCount);

            i32 newId = _occupyNextTextureSlot();
            outGpuTexHandles[r] = &texGpuId[newId];
            texGpuId[newId] = gpuTexDefault;
            texDiskId[newId] = reqPakTexId;
            texFramesNotRequested[newId] = 0;
            texLoaded[newId]._count = 0;

            TextureDesc2D& desc = texDesc[newId];

            // upload texture to gpu
            const i32 actualTexId = reqPakTexId - 1;
            desc.width = diskTextures->textureInfo[actualTexId].width;
            desc.height = diskTextures->textureInfo[actualTexId].height;
            desc.data =  diskTextures->textureData[actualTexId];

            if(diskTextures->textureInfo[actualTexId].type == DiskTextures::TYPE_RGBA8) {
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


            cmds.createTexture2D(&desc, &texGpuId[newId]);
            cmds.counterIncrement(&texLoaded[newId]);
        }
        renderer_pushCommandList(cmds);

#ifdef CONF_DEBUG
        for(i32 r = 0; r < requestCount; ++r) {
            assert(outGpuTexHandles[r] >= texGpuId && outGpuTexHandles[r] < &texGpuId[MAX_GPU_TEXTURES]);
            i32 id = outGpuTexHandles[r] - texGpuId;
            assert(inPakTextureIds[r] == texDiskId[id]);
        }
#endif*/
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
u8** textureDataPtr;
MemBlock textureOffsetBlock;

u8* textureAllData;
MemBlock textureAllDataBlock;

GPUResources gpu;

enum class LoadStatus: i32 {
    NONE = 0,
    LOADED,
    LOADING,
};

bool init()
{
    gpu.init();

    if(!fileOpenToRead("../sacred_data/texture.pak", &fileTexture)) {
        return false;
    }

    textureAllDataBlock = MEM_ALLOC(Megabyte(100));
    assert(textureAllDataBlock.ptr);
    textureAllData = (u8*)textureAllDataBlock.ptr;

    PakHeader header;
    fileReadAdvance(&fileTexture, sizeof(header), &header);
    textureCount = header.entryCount;

    // alloc texture metadata
    textureOffsetBlock = MEM_ALLOC((sizeof(i32) + sizeof(AtomicCounter) + sizeof(u8*)) * textureCount);
    assert(textureOffsetBlock.ptr);
    textureFileOffset = (i32*)textureOffsetBlock.ptr;
    textureDiskLoadStatus = (AtomicCounter*)(textureFileOffset + textureCount);
    textureDataPtr = (u8**)(textureDiskLoadStatus + textureCount);

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
    MEM_DEALLOC(textureOffsetBlock);
    MEM_DEALLOC(textureAllDataBlock);
    fileClose(&fileTexture);
    gpu.deinit();
}

void newFrame()
{
    // decompress texture if necessary and upload them to the gpu (if requested)
    for(i32 i = 0; i < textureCount; ++i) {
        if((LoadStatus)textureDiskLoadStatus[i].get() == LoadStatus::LOADED /*&& NOT GPU LOADING*/) {
            //pak_textureRead();
            //upload to GPU
        }
    }
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
    for(i32 i = 0; i < requestCount; ++i) {
        const i32 texUID = textureUIDs[i];
        assert(texUID > 0 && texUID < textureCount);
        out_gpuHandles[i] = &gpu.gpuTexDefault;

        if((LoadStatus)textureDiskLoadStatus[texUID].get() == LoadStatus::NONE && textureDataPtr[texUID] == nullptr) {
            textureDiskLoadStatus[texUID].set((i32)LoadStatus::LOADING);
            textureDataPtr[texUID] = (u8*)0xdeadbeef;

            i32 size = textureFileOffset[textureUIDs[i] + 1] - textureFileOffset[textureUIDs[i]];
            assert(size > 0);

            fileAsyncReadAbsolute(&fileTexture, textureFileOffset[textureUIDs[i]], size,
                    &textureDiskLoadStatus[texUID]);
        }
    }
}

void loadTexture(char* fileBuff, i64 buffSize)
{
    i32 width, height, type, textureSize;
    pak_textureRead(fileBuff, buffSize, &width, &height, &type, textureAllData, &textureSize);
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

static void resource_loadTexture(char* buff, i64 size)
{
    DR.loadTexture(buff, size);
}

u32 resource_defaultGpuTexture()
{
    return DR.gpu.gpuTexDefault;
}
