#pragma once
#include "gl3w.h"
#include "rs_base.h"
#include "rs_array.h"
#include "rs_thread.h"
#include "imgui.h"

#ifdef CONF_DEBUG
    #define OGL_DBG_GROUP_BEGIN(name) \
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, -(__COUNTER__^(__LINE__*1337)), strLen(#name), #name)
    #define OGL_DBG_GROUP_END(name) glPopDebugGroup()
#else
    #define OGL_DBG_GROUP_BEGIN(name)
    #define OGL_DBG_GROUP_END(name)
#endif

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

struct TextureDesc2D
{
    i32 width;
    i32 height;
    i32 minFilter = GL_LINEAR;
    i32 magFilter = GL_LINEAR;
    i32 wrapS = GL_CLAMP_TO_EDGE;
    i32 wrapT = GL_CLAMP_TO_EDGE;
    i32 internalFormat;
    i32 dataFormat;
    i32 dataPixelCompType = GL_UNSIGNED_BYTE;
    const void* data;
};

struct TileVertex
{
    f32 x, y, z;
    f32 u, v;
    f32 amu, amv; // alpha mask UV
    u32 color;

    TileVertex() = default;
    TileVertex(f32 _x, f32 _y, f32 _z, f32 _u, f32 _v, f32 _amu, f32 _amv, u32 _c) {
        x = _x;
        y = _y;
        z = _z;
        u = _u;
        v = _v;
        amu = _amu;
        amv = _amv;
        color = _c;
    }
};

#define MAX_GPU_TEXTURES 1024

struct RendererFrameData
{
    // gpu texture management
    u32 gpuTexDestroyList[MAX_GPU_TEXTURES];
    u32* gpuTexIdToCreate[MAX_GPU_TEXTURES];
    TextureDesc2D texDescToCreate[MAX_GPU_TEXTURES];
    i32 texDestroyCount = 0;
    i32 texToCreateCount = 0;

    Array<ImDrawList> imguiDrawList;

    inline void addTextureToDestroyList(u32 texture)
    {
        gpuTexDestroyList[texDestroyCount++] = texture;
    }

    inline void addTextureCreate(TextureDesc2D desc, u32* destId)
    {
        i32 i = texToCreateCount++;
        texDescToCreate[i] = desc;
        gpuTexIdToCreate[i] = destId;
    }

    void clear();

    RendererFrameData() = default;
    void copy(const RendererFrameData& other);
};

unsigned long thread_renderer(void*);
void renderer_waitForInit();
bool renderer_setupImGuiSync(struct ImGuiGLSetup* ims, u8* pFontPixels, i32 fontTexWidth, i32 fontTexHeight);

f64 renderer_getFrameTime();
void renderer_pushFrame(const RendererFrameData& frameData);
