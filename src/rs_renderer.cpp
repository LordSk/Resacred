#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_logger.h"
#include "rs_gl_utils.h"
#include "rs_array.h"
#include "gl3w.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include <stdio.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>

#include "shaders/fs_ocornut_imgui.bin.h"
#include "shaders/fs_imgui_image.bin.h"
#include "shaders/vs_ocornut_imgui.bin.h"
#include "shaders/vs_imgui_image.bin.h"

GLuint createTexture2D(const TextureDesc2D& desc)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc.magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, desc.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, desc.wrapT);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        desc.internalFormat,
        desc.width,
        desc.height,
        0,
        desc.dataFormat,
        desc.dataPixelCompType,
        desc.data
    );
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void getUniformLocations(GLint program, const char* names[], i32* locations[], const i32 count)
{
    for(i32 i = 0; i < count; ++i) {
        *locations[i] = glGetUniformLocation(program, names[i]);
        assert(*locations[i] != -1);
    }
}

void enableVertexAttribArrays(const i32* indices, const i32 count)
{
    for(i32 i = 0; i < count; ++i) {
        glEnableVertexAttribArray(indices[i]);
    }
}

void vertexAttribPointer(i32 index, i32 numComp, i32 type_, bool normalized, i32 stride, void* pointer)
{
    glVertexAttribPointer(
                index,
                numComp,
                type_,
                normalized,
                stride,
                pointer);
}

inline void bindArrayBuffer(GLuint buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
}

inline void blendModeOpaque()
{
    glDisable(GL_BLEND);
}

inline void blendModeTransparency()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

inline void textureSlot(bgfx::TextureHandle texture, i32 slot)
{
	// FIXME: reenable
	/*glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, texture);*/
}

inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexDecl& _decl, uint32_t _numIndices)
{
	return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _decl)
		&& (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
		;
}

struct ImGuiSetup
{
	bgfx::ProgramHandle colorProgram;
	bgfx::ProgramHandle imageProgram;
	bgfx::UniformHandle uImageLodEnabled;
	bgfx::UniformHandle uSampleTex;
	bgfx::TextureHandle texture;
	bgfx::VertexDecl decl;
	const int viewId = 255;
    f32 viewMatrix[16];
    timept lastFrameTime;

	bool init()
	{
		// setup imgui
		static const bgfx::EmbeddedShader s_embeddedShaders[] =
		{
			BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
			BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
			BGFX_EMBEDDED_SHADER(vs_imgui_image),
			BGFX_EMBEDDED_SHADER(fs_imgui_image),

			BGFX_EMBEDDED_SHADER_END()
		};

		bgfx::RendererType::Enum type = bgfx::getRendererType();
		colorProgram = bgfx::createProgram(
					bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
					bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
					true);

		uImageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
		imageProgram = bgfx::createProgram(
					bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
					bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"),
					true);

		decl.begin()
			.add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
		.end();

		uSampleTex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

		uint8_t* data;
		int32_t width;
		int32_t height;

		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

		texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(data, width*height*4));

		return true;
	}

	void render(ImDrawData* _drawData)
	{
		const ImGuiIO& io = ImGui::GetIO();
		const float width  = io.DisplaySize.x;
		const float height = io.DisplaySize.y;

		bgfx::setViewName(viewId, "ImGui");
		bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

		const bgfx::Caps* caps = bgfx::getCaps();
		{
			float ortho[16];
			bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
			bgfx::setViewTransform(viewId, NULL, ortho);
			bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height) );
		}

		// Render command lists
		for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii)
		{
			bgfx::TransientVertexBuffer tvb;
			bgfx::TransientIndexBuffer tib;

			const ImDrawList* drawList = _drawData->CmdLists[ii];
			uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
			uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

			if(!checkAvailTransientBuffers(numVertices, decl, numIndices) )
			{
				// not enough space in transient buffer just quit drawing the rest...
				break;
			}

			bgfx::allocTransientVertexBuffer(&tvb, numVertices, decl);
			bgfx::allocTransientIndexBuffer(&tib, numIndices);

			ImDrawVert* verts = (ImDrawVert*)tvb.data;
			bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert) );

			ImDrawIdx* indices = (ImDrawIdx*)tib.data;
			bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx) );

			uint32_t offset = 0;
			for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
			{
				if (cmd->UserCallback)
				{
					cmd->UserCallback(drawList, cmd);
				}
				else if (0 != cmd->ElemCount)
				{
					uint64_t state = 0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_MSAA
						;

					bgfx::TextureHandle th = texture;
					bgfx::ProgramHandle prog = colorProgram;

					if (NULL != cmd->TextureId)
					{
						union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
						state |= 0 != (0x1 & texture.s.flags)
							? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
							: BGFX_STATE_NONE
							;
						th = texture.s.handle;
						if (0 != texture.s.mip)
						{
							const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
							bgfx::setUniform(uImageLodEnabled, lodEnabled);
							prog = imageProgram;
						}
					}
					else
					{
						state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
					}

					const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f) );
					const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f) );
					bgfx::setScissor(xx, yy
						, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f)-xx)
						, uint16_t(bx::min(cmd->ClipRect.w, 65535.0f)-yy)
						);

					bgfx::setState(state);
					bgfx::setTexture(0, uSampleTex, th);
					bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
					bgfx::setIndexBuffer(&tib, offset, cmd->ElemCount);
					bgfx::submit(viewId, prog);
				}

				offset += cmd->ElemCount;
			}
		}
	}

	void cleanUp()
	{
		bgfx::destroy(colorProgram);
		bgfx::destroy(imageProgram);
		bgfx::destroy(uImageLodEnabled);
		bgfx::destroy(uSampleTex);
		bgfx::destroy(texture);
	}
};

struct TileShader
{
    GLuint program;
    i32 uProjMatrix;
    i32 uViewMatrix;
    i32 uModelMatrix;
    i32 uDiffuse;
    i32 uAlphaMask;
    GLuint vao;
    GLuint vbo;

    void loadAndCompile()
    {
        OGL_DBG_GROUP_BEGIN(TileShader);

        // ui shader
        constexpr const char* vertexShader = R"FOO(
            #version 330 core
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec2 uv;
            layout(location = 2) in vec2 am_uv;
            layout(location = 3) in vec4 color;
            uniform mat4 uProjMatrix;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec2 vert_uv;
            out vec2 vert_am_uv;
            flat out int vert_isAlphaMasked;
            out vec4 vert_color;

            void main()
            {
                vert_uv = uv;
                vert_am_uv = am_uv;
                vert_isAlphaMasked = int(am_uv.x != -1);
                vert_color = color;
                gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(position, 1.0);
            }
            )FOO";

        constexpr const char* fragmentShader = R"FOO(
            #version 330 core
            uniform sampler2D uDiffuse;
            uniform sampler2D uAlphaMask;

            in vec2 vert_uv;
            in vec2 vert_am_uv;
            flat in int vert_isAlphaMasked;
            in vec4 vert_color;
            out vec4 fragmentColor;

            void main()
            {
                vec4 diff = texture(uDiffuse, vert_uv);
                vec4 mask = texture(uAlphaMask, vert_am_uv);
                fragmentColor = diff * vert_color;
                fragmentColor.a = (1.0-vert_isAlphaMasked) * diff.a + (vert_isAlphaMasked * mask.a);
            }
            )FOO";

        i32 vertexShaderLen = strlen(vertexShader);
        i32 fragmentShaderLen = strlen(fragmentShader);

        GLShaderFile shaderFiles[2] = {
            {vertexShader, vertexShaderLen, GL_VERTEX_SHADER},
            {fragmentShader, fragmentShaderLen, GL_FRAGMENT_SHADER}
        };

        program = glMakeShader(shaderFiles, 2);

        static i32* locations[] = {&uProjMatrix, &uViewMatrix, &uModelMatrix, &uDiffuse, &uAlphaMask};
        static const char* uniformNames[] = {"uProjMatrix", "uViewMatrix", "uModelMatrix",
                                             "uDiffuse", "uAlphaMask"};
        glUseProgram(program);
        getUniformLocations(program, uniformNames, locations, IM_ARRAYSIZE(locations));

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        bindArrayBuffer(vbo);

        enum Location {
            POSITION = 0,
            UV,
            AM_UV,
            COLOR,
        };

        i32 indices[] = {POSITION, UV, AM_UV, COLOR};
        enableVertexAttribArrays(indices, sizeof(indices)/sizeof(Location));

        vertexAttribPointer(Location::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, x));
        vertexAttribPointer(Location::UV, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, u));
        vertexAttribPointer(Location::AM_UV, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, amu));
        vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TileVertex),
                                (GLvoid*)OFFSETOF(TileVertex, color));

        OGL_DBG_GROUP_END(TileShader);
    }
};


struct DbgColorShader
{
    GLuint program;
    i32 uProjMatrix;
    i32 uViewMatrix;
    i32 uModelMatrix;
    GLuint vao;
    GLuint vbo_vertexData;

    bool loadAndCompile()
    {
        OGL_DBG_GROUP_BEGIN(DbgColorShader_setup);

        // ui shader
        constexpr const char* vertexShader = R"FOO(
            #version 330 core
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec4 color;
            uniform mat4 uProjMatrix;
            uniform mat4 uViewMatrix;
            uniform mat4 uModelMatrix;

            out vec4 vert_color;

            void main()
            {
                vert_color = color;
                gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(position, 1.0);
            }
            )FOO";

        constexpr const char* fragmentShader = R"FOO(
            #version 330 core

            in vec4 vert_color;
            out vec4 fragmentColor;

            void main()
            {
                fragmentColor = vert_color;
            }
            )FOO";

        i32 vertexShaderLen = strlen(vertexShader);
        i32 fragmentShaderLen = strlen(fragmentShader);

        GLShaderFile shaderFiles[2] = {
            {vertexShader, vertexShaderLen, GL_VERTEX_SHADER},
            {fragmentShader, fragmentShaderLen, GL_FRAGMENT_SHADER}
        };

        program = glMakeShader(shaderFiles, 2);
        if(!program) {
            return false;
        }

        static i32* locations[] = {&uProjMatrix, &uViewMatrix, &uModelMatrix};
        static const char* uniformNames[] = {"uProjMatrix", "uViewMatrix", "uModelMatrix"};

        glUseProgram(program);
        getUniformLocations(program, uniformNames, locations, IM_ARRAYSIZE(locations));

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo_vertexData);
        bindArrayBuffer(vbo_vertexData);

        enum Location {
            POSITION = 0,
            COLOR = 1,
        };

        const i32 indexes[] = {POSITION, COLOR};
        enableVertexAttribArrays(indexes, IM_ARRAYSIZE(indexes));

        vertexAttribPointer(Location::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                            (GLvoid*)OFFSETOF(QuadVertex, x));
        vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex),
                            (GLvoid*)OFFSETOF(QuadVertex, color));

        OGL_DBG_GROUP_END(DbgColorShader_setup);
        return true;
    }
};

void RendererFrameData::clear()
{
    texToDestroyCount = 0;
    texToCreateCount = 0;
    doUploadTileVertexData = false;
	// FIXME: reenable
	//imguiDrawList.clear();
    tileVertexData.clearPOD();
    tileQuadGpuTex.clearPOD();
    textureData.clearPOD();

    dbgQuadVertData.clearPOD();
    dbgQuadMeshDef.clearPOD();
    dbgQuadModelMat.clearPOD();

    tvOff_base = -1;
}

void RendererFrameData::copy(const RendererFrameData& other)
{
     // there is a lot going on here (C++ copy)
    *this = other;
}

struct Renderer
{

i32 fillingQueueId = 0;
f64 frameTime = 0;
timept framet0 = timeNow();
ImGuiSetup imguiSetup;

bool initialized = false;
Mutex queueMutex;

TileShader shader_tile;
DbgColorShader shader_dbgColor;

RendererFrameData frameData[2];
MutexSpin frameDataMutex[2];
bool frameReady[2] = {0};
i32 backFrameId = 0;

VramInfo vramInfo;
i32 gpuTileVertexCount = 4096;
i32 gpuDbgQuadVertexCount = 4096;

bool init()
{
    LOG("Renderer> initialization...");

    Window& client = *get_clientWindow();

	bgfx::PlatformData pd;
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(client.window, &wmInfo);
	pd.nwh = wmInfo.info.win.window;

	bgfx::Init bgfxInit;
	//bgfxInit.type = bgfx::RendererType::Count; // Automatically choose a renderer.
	bgfxInit.type = bgfx::RendererType::Direct3D12;
	bgfxInit.resolution.width = client.width;
	bgfxInit.resolution.height = client.height;
	bgfxInit.resolution.reset = BGFX_RESET_NONE; // no vsync
	bgfxInit.platformData = pd;
#ifndef CONF_DEBUG
	bgfxInit.debug = false;
	bgfxInit.profile = false;
#endif
	bgfx::init(bgfxInit);

	bgfx::setDebug(BGFX_DEBUG_TEXT);

	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x3F3F3FFF, 1.0f, 0);
	bgfx::setViewRect(0, 0, 0, client.width, client.height);

	imguiSetup.init();

	/*shader_tile.loadAndCompile();
    // TODO: make a bunch of buffers for each sector and update only on loading
    glBindBuffer(GL_ARRAY_BUFFER, shader_tile.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * gpuTileVertexCount, nullptr,
                 GL_STATIC_DRAW);

    if(!shader_dbgColor.loadAndCompile()) {
        return false;
    }

    bindArrayBuffer(shader_dbgColor.vbo_vertexData);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * gpuDbgQuadVertexCount, nullptr,
				 GL_DYNAMIC_DRAW);

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glDisable(GL_CULL_FACE);
    //glEnable(GL_DEPTH_TEST);

    //glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/
    initialized = true;
    return true;
}

void frameDoTextureManagement(RendererFrameData& frame)
{
    glDeleteTextures(frame.texToDestroyCount, frame.gpuTexDestroyList);

    // FIXME: breaks when limiting texture count per frame
    const i32 createCount = frame.texToCreateCount;
    TextureDesc2D* descs = frame.texDescToCreate;
    u8* texData = frame.textureData.data();
    i32* texDataOffset = frame.texDescDataOfset;
    u32** gpuTexIdToCreate = frame.gpuTexIdToCreate;

    for(i32 i = 0; i < createCount; ++i) {
        TextureDesc2D& d = descs[i];
        d.data = texData + texDataOffset[i];
        *(gpuTexIdToCreate[i]) = createTexture2D(d);
    }
}

void frameDoSectorRender(const RendererFrameData& frame)
{
    if(frame.tvOff_base < 0) return; // TODO: remove this

    OGL_DBG_GROUP_BEGIN(SectorRender);

    glUseProgram(shader_tile.program);
    glUniform1i(shader_tile.uDiffuse, 0);
    glUniform1i(shader_tile.uAlphaMask, 1);
    glUniformMatrix4fv(shader_tile.uProjMatrix, 1, GL_FALSE, frame.matCamProj.data);
    glUniformMatrix4fv(shader_tile.uViewMatrix, 1, GL_FALSE, frame.viewIsIso ? frame.matCamViewIso.data:
                                                                               frame.matCamViewOrtho.data);
    glUniformMatrix4fv(shader_tile.uModelMatrix, 1, GL_FALSE, frame.matSectorTileModel.data);

    bindArrayBuffer(shader_tile.vbo);

    // upload vertex data to gpu
    const i32 tileVertexDataCount = frame.tileVertexData.count();
    const TileVertex* tileVertexData = frame.tileVertexData.data();

    if(frame.doUploadTileVertexData) {
        if(tileVertexDataCount > gpuTileVertexCount) {
			gpuTileVertexCount = MAX(gpuTileVertexCount * 2, tileVertexDataCount);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * gpuTileVertexCount, 0, GL_DYNAMIC_DRAW);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileVertex) * tileVertexDataCount,
                        tileVertexData);
    }

    glBindVertexArray(shader_tile.vao);

    blendModeOpaque();

    const i32 baseEnd = frame.tvOff_floor;
    for(i32 i = 0; i < baseEnd; i += 6) {
		textureSlot(frame.tileQuadGpuTex[i/6], 0);
        glDrawArrays(GL_TRIANGLES, i, 6);
    }

    blendModeTransparency();

    const i32 floorEnd = frame.tvOff_mixed;
    const i32 alphaMaskTexOffset = (floorEnd - baseEnd) / 6;
    for(i32 i = baseEnd; i < floorEnd; i += 6) {
		textureSlot(frame.tileQuadGpuTex[i/6], 0);
		textureSlot(frame.tileQuadGpuTex[i/6 + alphaMaskTexOffset], 1);
        glDrawArrays(GL_TRIANGLES, i, 6);
    }

    glUniformMatrix4fv(shader_tile.uViewMatrix, 1, GL_FALSE, frame.matCamViewOrtho.data);
    glUniformMatrix4fv(shader_tile.uModelMatrix, 1, GL_FALSE, frame.matSectorMixedModel.data);

    const i32 mixedEnd = tileVertexDataCount;
    for(i32 i = floorEnd; i < mixedEnd; i += 6) {
		textureSlot(frame.tileQuadGpuTex[alphaMaskTexOffset + i/6], 0);
        glDrawArrays(GL_TRIANGLES, i, 6);
    }

    OGL_DBG_GROUP_END(SectorRender);
}

void frameDoDbgObjects(const RendererFrameData& frame)
{
    OGL_DBG_GROUP_BEGIN(DbgObjects);

    glUseProgram(shader_dbgColor.program);

    glUniformMatrix4fv(shader_dbgColor.uProjMatrix, 1, GL_FALSE, frame.matCamProj.data);
    glUniformMatrix4fv(shader_dbgColor.uViewMatrix, 1, GL_FALSE, frame.matCamViewIso.data);
    glUniformMatrix4fv(shader_dbgColor.uViewMatrix, 1, GL_FALSE, frame.viewIsIso ? frame.matCamViewIso.data :
                                                                             frame.matCamViewOrtho.data);

    bindArrayBuffer(shader_dbgColor.vbo_vertexData);
    const i32 dbgQuadVertCount = frame.dbgQuadVertData.count();
    if(dbgQuadVertCount > gpuDbgQuadVertexCount) {
		gpuDbgQuadVertexCount = MAX(gpuDbgQuadVertexCount * 2, dbgQuadVertCount);
        glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * gpuDbgQuadVertexCount, 0, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(QuadVertex) * dbgQuadVertCount,
                    frame.dbgQuadVertData.data());

    glBindVertexArray(shader_dbgColor.vao);

    blendModeTransparency();

    const mat4* matModels = frame.dbgQuadModelMat.data();
    const MeshDef* meshDef = frame.dbgQuadMeshDef.data();
    const i32 meshCount = frame.dbgQuadMeshDef.count();

    assert(meshCount == frame.dbgQuadModelMat.count());

    for(i32 i = 0; i < meshCount; ++i) {
        glUniformMatrix4fv(shader_dbgColor.uModelMatrix, 1, GL_FALSE, matModels[i].data);
        glDrawArrays(GL_TRIANGLES, meshDef[i].vertOffset, meshDef[i].vertCount);
    }


    OGL_DBG_GROUP_END(DbgObjects);
}

void processFrames()
{
    Window& client = *get_clientWindow();
	while(client.isRunning) {
		/*timept t0 = timeNow();
        while(!frameReady[backFrameId]) {
            _mm_pause();
        }

        MutexSpin& frameMutex = frameDataMutex[backFrameId];
        frameMutex.lock();
        frameReady[backFrameId] = false;
        RendererFrameData& curFrame = frameData[backFrameId];
        backFrameId ^= 1;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &vramInfo.dedicated);
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &vramInfo.availMemory);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &vramInfo.currentAvailMem);
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &vramInfo.evictionCount);
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &vramInfo.evictedMem);

        frameDoTextureManagement(curFrame);

        frameDoSectorRender(curFrame);
        frameDoDbgObjects(curFrame);
        frameDoImGui(curFrame);

        if(!get_clientWindow()) {
            return;
        }
        client.swapBuffers();

        curFrame.clear();
        frameMutex.unlock();
		frameTime = timeDuration(timeNow() - t0);*/
    }
}

void cleanUp()
{
    LOG_DBG("Renderer> cleaning up...");
	imguiSetup.cleanUp();
	bgfx::shutdown();
}

void pushFrame(const RendererFrameData& frameData_)
{
#ifdef CONF_DEBUG
    timept t0 = timeNow();
#endif

    // prevent important frames from being squashed
    i32 bid = backFrameId;
    while(frameData[bid].doUploadTileVertexData || frameData[bid].texToCreateCount > 0
          || frameData[bid].texToDestroyCount > 0) {
        _mm_pause();
        //assert(timeDurSince(t0) < 1.0);
        bid = backFrameId;
    }

    MutexSpin& frameMutex = frameDataMutex[bid];
    frameMutex.lock();
    frameData[bid].copy(frameData_);

    /*
    assert(frameData_.imguiDrawList.data() != frameData[backFrameId].imguiDrawList.data());

    for(i32 i = 0; i < frameData_.imguiDrawList.count(); i++) {
        assert(frameData[bid].imguiDrawList[i].VtxBuffer.size() ==
               frameData_.imguiDrawList[i].VtxBuffer.size());
        assert(frameData[bid].imguiDrawList[i].VtxBuffer.Data !=
               frameData_.imguiDrawList[i].VtxBuffer.Data);
        assert(frameData[bid].imguiDrawList[i].VtxBuffer.size() > 0);
    }*/

    frameReady[bid] = true;
    frameMutex.unlock();
}

};

static Renderer* g_rendererPtr = nullptr;

bool renderer_init()
{
	static Renderer r;
	g_rendererPtr = &r;
	return r.init();
}

void renderer_shutdown()
{
	g_rendererPtr->cleanUp();
}

void renderer_waitForInit()
{
    while(!g_rendererPtr || !g_rendererPtr->initialized) {
        _mm_pause();
    }
}

f64 renderer_getFrameTime()
{
    return g_rendererPtr->frameTime;
}

void renderer_pushFrame(const RendererFrameData& frameData)
{
    return g_rendererPtr->pushFrame(frameData);
}

VramInfo renderer_getVramInfo()
{
    return g_rendererPtr->vramInfo;
}

void renderer_renderDbgUi()
{
	ImGui::Render();
	g_rendererPtr->imguiSetup.render(ImGui::GetDrawData());
}

void renderer_frame()
{
	bgfx::frame();
}
