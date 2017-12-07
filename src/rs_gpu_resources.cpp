#include "rs_gpu_resources.h"
#include "rs_file.h"
#include "rs_renderer.h"
#include "imgui.h"

#define MAX_GPU_TEXTURES 1024

struct GPUResources
{
    DiskTextures* diskTextures;
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
        texGpuId[oldestId] = 0;
        texDiskId[oldestId] = 0;
        texLoaded[oldestId]._count = 0;
        texSlotOccupied[oldestId] = true;
        return oldestId;
    }

    void requestTextures(i32* inPakTextureIds, u32** outGpuTexHandles, const i32 requestCount)
    {
        memset(outGpuTexHandles, 0, sizeof(outGpuTexHandles[0]) * requestCount);

        // assign already loaded/loading textures
        for(i32 r = 0; r < requestCount; ++r) {
            const i32 pakTexId = inPakTextureIds[r];
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

            const i32 pakTexId = inPakTextureIds[r];
            i32 newId = _occupyNextTextureSlot();
            outGpuTexHandles[r] = &texGpuId[newId];
            texDiskId[newId] = pakTexId;
            texFramesNotRequested[newId] = 0;
            texLoaded[newId]._count = 0;

            TextureDesc2D& desc = texDesc[newId];

            // upload texture to gpu
            desc.width = diskTextures->textureInfo[pakTexId].width;
            desc.height = diskTextures->textureInfo[pakTexId].height;
            desc.data =  diskTextures->textureData[pakTexId];

            if(diskTextures->textureInfo[pakTexId].type == DiskTextures::TYPE_RGBA8) {
                desc.internalFormat = GL_RGBA8;
                desc.dataFormat = GL_RGBA;
                desc.dataPixelCompType = GL_UNSIGNED_BYTE;
            }
            else {
                desc.internalFormat = GL_RGBA4;
                desc.dataFormat = GL_BGRA;
                desc.dataPixelCompType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            }

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
#endif
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
                texGpuId[i], texDiskId[i], texFramesNotRequested[i]);
        }
        ImGui::End();

        ImGui::End();
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

void GPUres_debugUi()
{
    assert(g_gpuResourcesPtr);
    g_gpuResourcesPtr->debugUi();
}
