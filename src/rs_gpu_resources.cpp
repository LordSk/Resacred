#include "rs_gpu_resources.h"
#include "rs_file.h"
#include "rs_renderer.h"

#define MAX_TEXTURE_IDS 1024

struct GPUResources
{
    DiskTextures* diskTextures;
    u32 texGpuId[MAX_TEXTURE_IDS];
    i32 texDiskId[MAX_TEXTURE_IDS];
    i32 texFramesNotRequested[MAX_TEXTURE_IDS];
    u8 texSlotOccupied[MAX_TEXTURE_IDS];
    TextureDesc2D texDesc[MAX_TEXTURE_IDS];
    i32 textureCount = 0;

    bool init()
    {
        textureCount = 0;
        memset(texFramesNotRequested, 0, sizeof(texFramesNotRequested));
        memset(texSlotOccupied, 0, sizeof(texSlotOccupied));
        return true;
    }

    void newFrame()
    {
        CommandList cmds;

        // find unused textures and destroy them
        for(i32 i = 0; i < MAX_TEXTURE_IDS; ++i) {
            if(texSlotOccupied[i]) {
                texFramesNotRequested[i]++;

                if(texFramesNotRequested[i] > 10) {
                    cmds.destroyTexture(texGpuId[i]);
                    texGpuId[i] = 0;
                    texDiskId[i] = 0;
                    texSlotOccupied[i] = false;
                }
            }
        }

        renderer_pushCommandList(cmds);
    }

    i32 _occupyNextTextureSlot()
    {
        for(i32 i = 0; i < MAX_TEXTURE_IDS; ++i) {
            if(texSlotOccupied[i] == false) {
                texSlotOccupied[i] = true;
                return i;
            }
        }

        assert(false);
        return -1;
    }

    void requestTextures(i32* pakTextureIds, u32** outGpuTexHandles, const i32 requestCount)
    {
        memset(outGpuTexHandles, 0, sizeof(i32) * requestCount);

        for(i32 r = 0; r < requestCount; ++r) {
            assert(pakTextureIds[r] > 0 && pakTextureIds[r] < diskTextures->textureCount);
            for(i32 i = 0; i < MAX_TEXTURE_IDS; ++i) {
                if(texSlotOccupied[i] && texDiskId[i] == pakTextureIds[r]) {
                    outGpuTexHandles[r] = &texGpuId[i];
                    texFramesNotRequested[i] = 0;
                }
            }
        }

        CommandList cmds;
        for(i32 r = 0; r < requestCount; ++r) {
            if(outGpuTexHandles[r] != 0) {
                continue;
            }

            const i32 pakTexId = pakTextureIds[r];
            i32 newId = _occupyNextTextureSlot();
            outGpuTexHandles[r] = &texGpuId[newId];
            texDiskId[newId] = pakTexId;
            texFramesNotRequested[newId] = 0;

            // upload texture to gpu
            texDesc[newId].width = diskTextures->textureInfo[pakTexId].width;
            texDesc[newId].height = diskTextures->textureInfo[pakTexId].height;
            texDesc[newId].data =  diskTextures->textureData[pakTexId];

            if(diskTextures->textureInfo[pakTexId].type == DiskTextures::TYPE_RGBA8) {
                texDesc[newId].internalFormat = GL_RGBA8;
                texDesc[newId].dataFormat = GL_RGBA;
                texDesc[newId].dataPixelCompType = GL_UNSIGNED_BYTE;
            }
            else {
                texDesc[newId].internalFormat = GL_RGBA4;
                texDesc[newId].dataFormat = GL_BGRA;
                texDesc[newId].dataPixelCompType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            }

            cmds.createTexture2D(&texDesc[newId], &texGpuId[newId]);
        }
        renderer_pushCommandList(cmds);
    }

    void deinit()
    {
        CommandList cmds;
        for(i32 i = 0; i < MAX_TEXTURE_IDS; ++i) {
            if(texSlotOccupied[i]) {
                cmds.destroyTexture(texGpuId[i]);
            }
        }
        cmds.execute();
        renderer_pushCommandList(cmds);
    }
};

GPUResources* g_gpuResourcesPtr = nullptr;

bool GPUres_init(DiskTextures* diskTextures)
{
    assert(!g_gpuResourcesPtr);
    static GPUResources gpuRes;
    g_gpuResourcesPtr = &gpuRes;
    gpuRes.diskTextures = diskTextures;
    return gpuRes.init();
}

void GPUres_newFrame()
{
    assert(g_gpuResourcesPtr);
    g_gpuResourcesPtr->newFrame();
}

void GPUres_requestTextures(i32* pakTextureIds, u32** outGpuTexHandles, const i32 requestCount)
{
    assert(g_gpuResourcesPtr);
    g_gpuResourcesPtr->requestTextures(pakTextureIds, outGpuTexHandles, requestCount);
}

void GPUres_deinit()
{
    assert(g_gpuResourcesPtr);
    g_gpuResourcesPtr->deinit();
}
