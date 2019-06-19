#pragma once
#include "gl3w.h"
#include "rs_base.h"
#include "rs_array.h"
#include "rs_thread.h"
#include "rs_math.h"
#include "imgui.h"
#include <bgfx/bgfx.h>

// TODO: move
namespace ImGui
{
#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	inline void Image(bgfx::TextureHandle _handle
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
		, const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		, const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		)
	{
		union { struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; ImTextureID ptr; } texture;
		texture.s.handle = _handle;
		texture.s.flags  = _flags;
		texture.s.mip    = _mip;
		Image(texture.ptr, _size, _uv0, _uv1, _tintCol, _borderCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::Image.
	inline void Image(bgfx::TextureHandle _handle
		, const ImVec2& _size
		, const ImVec2& _uv0       = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1       = ImVec2(1.0f, 1.0f)
		, const ImVec4& _tintCol   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		, const ImVec4& _borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		)
	{
		Image(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _tintCol, _borderCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	inline bool ImageButton(bgfx::TextureHandle _handle
		, uint8_t _flags
		, uint8_t _mip
		, const ImVec2& _size
		, const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
		, int _framePadding      = -1
		, const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		, const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		)
	{
		union { struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; ImTextureID ptr; } texture;
		texture.s.handle = _handle;
		texture.s.flags  = _flags;
		texture.s.mip    = _mip;
		return ImageButton(texture.ptr, _size, _uv0, _uv1, _framePadding, _bgCol, _tintCol);
	}

	// Helper function for passing bgfx::TextureHandle to ImGui::ImageButton.
	inline bool ImageButton(bgfx::TextureHandle _handle
		, const ImVec2& _size
		, const ImVec2& _uv0     = ImVec2(0.0f, 0.0f)
		, const ImVec2& _uv1     = ImVec2(1.0f, 1.0f)
		, int _framePadding      = -1
		, const ImVec4& _bgCol   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
		, const ImVec4& _tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
		)
	{
		return ImageButton(_handle, IMGUI_FLAGS_ALPHA_BLEND, 0, _size, _uv0, _uv1, _framePadding, _bgCol, _tintCol);
	}

	inline void NextLine()
	{
		SetCursorPosY(GetCursorPosY() + GetTextLineHeightWithSpacing() );
	}

	inline bool MouseOverArea()
	{
		return false
			|| ImGui::IsAnyItemHovered()
			|| ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
			;
	}

} // namespace ImGui


/*#ifdef CONF_DEBUG
    #define OGL_DBG_GROUP_BEGIN(name) \
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, -(__COUNTER__^(__LINE__*1337)), strLen(#name), #name)
    #define OGL_DBG_GROUP_END(name) glPopDebugGroup()
#else*/
    #define OGL_DBG_GROUP_BEGIN(name)
    #define OGL_DBG_GROUP_END(name)
//#endif

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

struct QuadVertex
{
    f32 x, y, z;
    u32 color;

    QuadVertex() = default;
    QuadVertex(f32 _x, f32 _y, f32 _z, u32 _c) {
        x = _x;
        y = _y;
        z = _z;
        color = _c;
    }
};

struct MeshDef
{
    u32 vertOffset;
    u32 vertCount;
};

#define MAX_GPU_TEXTURES 1024

struct RendererFrameData
{
    // gpu texture management
    u32 gpuTexDestroyList[MAX_GPU_TEXTURES];
    u32* gpuTexIdToCreate[MAX_GPU_TEXTURES];
    TextureDesc2D texDescToCreate[MAX_GPU_TEXTURES];
    i32 texDescDataOfset[MAX_GPU_TEXTURES];
    Array<u8> textureData;
    i32 texToDestroyCount = 0;
    i32 texToCreateCount = 0;

    // imgui draw data
	// FIXME: reenable
	//Array<ImDrawList> imguiDrawList;

    // world draw data
    Array<TileVertex> tileVertexData;
	Array<bgfx::TextureHandle> tileQuadGpuTex;
    bool doUploadTileVertexData = false;
    // FIXME: we can overwrite frames if the renderer
    // is much slower than the game loop, thus never uploading the data
    // (we want this true for only one frame)
    // check if frame is mandatory?
    // -> see pushFrame(frame)

    bool viewIsIso; // TODO: remove, renderer should not know about this
    mat4 matCamProj;
    mat4 matCamViewIso;
    mat4 matCamViewOrtho;
    mat4 matSectorTileModel; // TODO: use a bunch of model matrices
    mat4 matSectorMixedModel;

    i32 tvOff_base;
    i32 tvOff_floor;
    i32 tvOff_mixed;

    Array<QuadVertex> dbgQuadVertData;
    Array<MeshDef> dbgQuadMeshDef;
    Array<mat4> dbgQuadModelMat;

	RendererFrameData() = default;
    void copy(const RendererFrameData& other);
    void clear();

    // functions used by GPUResources
    inline void _addTextureToDestroyList(u32 texture)
    {
        gpuTexDestroyList[texToDestroyCount++] = texture;
    }

    inline void _addTextureCreate(TextureDesc2D desc, u32* destId, void* textureData_, u32 textureDataSize)
    {
        i32 i = texToCreateCount++;
        texDescToCreate[i] = desc;
        gpuTexIdToCreate[i] = destId;
        texDescDataOfset[i] = textureData.count();
        textureData.pushPOD((u8*)textureData_, textureDataSize);
    }
};

struct VramInfo
{
    i32 dedicated;
    i32 availMemory;
    i32 currentAvailMem;
    i32 evictionCount;
    i32 evictedMem;
};

unsigned long thread_renderer(void*);
bool renderer_init();
void renderer_cleanUp();
void renderer_waitForInit();
bool renderer_setupImGuiSync(struct ImGuiGLSetup* ims, u8* pFontPixels, i32 fontTexWidth, i32 fontTexHeight);

f64 renderer_getFrameTime();
VramInfo renderer_getVramInfo();
void renderer_pushFrame(const RendererFrameData& frameData);

void renderer_renderDbgUi();
void renderer_frame();
