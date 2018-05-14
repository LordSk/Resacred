#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_logger.h"
#include "rs_gl_utils.h"
#include "rs_array.h"
#include "gl3w.h"
#include "imgui.h"
#include "imgui_sdl2_setup.h"
#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>

#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B

void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length, const GLchar* message,
                            const void* userParam)
{
    constexpr i32 MAX_SCOPES = 8;
    static i32 currentScopeId = -1;
    static char currentGroupName[MAX_SCOPES][256];

    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION && (*(i32*)&id) < 0) {
        if(type == GL_DEBUG_TYPE_PUSH_GROUP) {
            currentScopeId++;
            assert(currentScopeId < MAX_SCOPES);
            memmove(currentGroupName[currentScopeId], message, length);
            currentGroupName[currentScopeId][length] = 0;
            //LOG_DBG("push: %s id=%d", message, id);
        }
        else if(type == GL_DEBUG_TYPE_POP_GROUP) {
            currentGroupName[currentScopeId][0] = 0;
            currentScopeId--;
            assert(currentScopeId >= -1);
        }
    }

    switch(source) {
        case GL_DEBUG_SOURCE_API:
            source = 0;
        break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source = 1;
        break;
        case GL_DEBUG_SOURCE_OTHER:
            source = 2;
        break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source = 3;
        break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source = 4;
        break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source = 5;
        break;
    }
    constexpr const char* sourceStr[] = {
        "SRC_API",
        "SRC_APPLICATION",
        "SRC_OTHER",
        "SRC_SHADER_COMPILER",
        "SRC_THIRD_PARTY",
        "SRC_WINDOW_SYSTEM"
    };

    switch(type) {
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type = 0;
        break;
        case GL_DEBUG_TYPE_ERROR:
            type = 1;
        break;
        case GL_DEBUG_TYPE_MARKER:
            type = 2;
        break;
        case GL_DEBUG_TYPE_OTHER:
            type = 3;
        break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type = 4;
        break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type = 5;
        break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type = 6;
        break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type = 7;
        break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type = 8;
        break;
    }
    constexpr const char* typeStr[] = {
        "TYPE_DEPRECATED_BEHAVIOR",
        "TYPE_ERROR",
        "TYPE_MARKER",
        "TYPE_OTHER",
        "TYPE_PERFORMANCE",
        "TYPE_POP_GROUP",
        "TYPE_PORTABILITY",
        "TYPE_PUSH_GROUP",
        "TYPE_UNDEFINED_BEHAVIOR"
    };

    switch(severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity = 0;
        break;
        case GL_DEBUG_SEVERITY_LOW:
            severity = 1;
        break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity = 2;
        break;
        case GL_DEBUG_SEVERITY_HIGH:
            severity = 3;
        break;
    }
    constexpr const char* severityStr[] = {
        "SEV_NOTIFICATION",
        "SEV_LOW",
        "SEV_MEDIUM",
        "SEV_HIGH",
    };

    if(severity < 1) {
        return;
    }

    i32 color = LOG_COLOR_WARN;
    if(severity > 2) {
        color = LOG_COLOR_ERROR;
    }
    getGlobalLogger().logf(color, __FILE__, __LINE__,
                           "OGL> {%s|%s|%s} id=%x group=%s \"%s\"",
                           sourceStr[source], typeStr[type],
                           severityStr[severity], id, currentGroupName[currentScopeId], message);
    assert(severity <= 2);
}

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

inline void textureSlot(u32 texture, i32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
}

struct ImGuiGLSetup
{
    GLuint shaderProgram = 0;
    GLint shaderViewUni = -1;
    GLint shaderTextureUni = -1;
    GLuint shaderVertexBuff = 0;
    GLuint shaderElementsBuff = 0;
    GLuint shaderVao = 0;
    f32 viewMatrix[16];
    timept lastFrameTime;
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

    void loadAndCompile()
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
            MODEL = 2, // size 4 (matrix4x4)
        };

        const i32 indexes[] = {POSITION, COLOR, MODEL, MODEL+1, MODEL+2, MODEL+3};
        enableVertexAttribArrays(indexes, IM_ARRAYSIZE(indexes));

        vertexAttribPointer(Location::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                            (GLvoid*)OFFSETOF(QuadVertex, x));
        vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex),
                            (GLvoid*)OFFSETOF(QuadVertex, color));

        OGL_DBG_GROUP_END(DbgColorShader_setup);
    }
};

void RendererFrameData::clear()
{
    texToDestroyCount = 0;
    texToCreateCount = 0;
    doUploadTileVertexData = false;
    imguiDrawList.clear();
    tileVertexData.clearPOD();
    tileQuadGpuTex.clearPOD();
    textureData.clearPOD();

    dbgQuadVertData.clearPOD();
    dbgQuadMeshDef.clearPOD();
    dbgQuadModelMat.clearPOD();
}

void RendererFrameData::copy(const RendererFrameData& other)
{
     // there is a lot going on here (C++ copy)
    *this = other;
}

struct Renderer
{

SDL_GLContext glContext;
i32 fillingQueueId = 0;
f64 frameTime = 0;
timept framet0 = timeNow();
ImGuiGLSetup imguiSetup;

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
    glContext = SDL_GL_CreateContext(client.window);
    if(!glContext) {
        LOG("ERROR: can't create OpenGL 3.3 context (%s)",  SDL_GetError());
        return false;
    }

    if(gl3w_init()) {
        LOG("ERROR: can't init gl3w");
        return false;
    }

    if(!gl3w_is_supported(3, 3)) {
        LOG("ERROR: OpenGL 3.3 isn't available on this system");
        return false;
    }

    SDL_GL_SetSwapInterval(0); // no vsync

#ifdef CONF_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, this);
#endif

    shader_tile.loadAndCompile();
    // TODO: make a bunch of buffers for each sector and update only on loading
    glBindBuffer(GL_ARRAY_BUFFER, shader_tile.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * gpuTileVertexCount, nullptr,
                 GL_STATIC_DRAW);

    shader_dbgColor.loadAndCompile();
    bindArrayBuffer(shader_dbgColor.vbo_vertexData);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * gpuDbgQuadVertexCount, nullptr,
                 GL_DYNAMIC_DRAW);

    ImGuiIO& io = ImGui::GetIO();
    u8* pFontPixels;
    i32 fontTexWidth, fontTexHeight;
    io.Fonts->GetTexDataAsRGBA32(&pFontPixels, &fontTexWidth, &fontTexHeight);

    OGL_DBG_GROUP_BEGIN(SetupImGui);

    imguiSetup.lastFrameTime = timeNow();

    TextureDesc2D fontTexDesc;
    fontTexDesc.minFilter = GL_NEAREST;
    fontTexDesc.magFilter = GL_NEAREST;
    fontTexDesc.wrapS = GL_CLAMP_TO_EDGE;
    fontTexDesc.wrapT = GL_CLAMP_TO_EDGE;
    fontTexDesc.internalFormat = GL_RGBA;
    fontTexDesc.dataFormat = GL_RGBA;
    fontTexDesc.dataPixelCompType = GL_UNSIGNED_BYTE;
    fontTexDesc.data = pFontPixels;
    fontTexDesc.width = fontTexWidth;
    fontTexDesc.height = fontTexHeight;

    GLuint fontTexture = createTexture2D(fontTexDesc);
    io.Fonts->SetTexID((void*)(intptr_t)fontTexture);

    // ui shader
    constexpr const char* vertexShader = R"FOO(
        #version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 uv;
        layout(location = 2) in vec4 color;
        uniform mat4 uViewMatrix;

        out vec2 vert_uv;
        out vec4 vert_color;

        void main()
        {
            vert_uv = uv;
            vert_color = color;
            gl_Position = uViewMatrix * vec4(position, 0.0, 1.0);
        }
        )FOO";

    constexpr const char* fragmentShader = R"FOO(
        #version 330 core
        uniform sampler2D uTextureData;

        in vec2 vert_uv;
        in vec4 vert_color;
        out vec4 fragmentColor;

        void main()
        {
            fragmentColor = texture(uTextureData, vert_uv) * vert_color;
        }
        )FOO";

    i32 vertexShaderLen = strlen(vertexShader);
    i32 fragmentShaderLen = strlen(fragmentShader);

    GLShaderFile shaderFiles[2] = {
        {vertexShader, vertexShaderLen, GL_VERTEX_SHADER},
        {fragmentShader, fragmentShaderLen, GL_FRAGMENT_SHADER}
    };

    imguiSetup.shaderProgram = glMakeShader(shaderFiles, 2);

    if(!fontTexture || !imguiSetup.shaderProgram) {
        LOG_ERR("ERROR> setupImGuiSync() failed");
        return false;
    }

    i32* locations[] = {&imguiSetup.shaderViewUni, &imguiSetup.shaderTextureUni};
    const char* uniformNames[] = {"uViewMatrix", "uTextureData"};
    getUniformLocations(imguiSetup.shaderProgram, uniformNames, locations, 2);

    glGenBuffers(1, &imguiSetup.shaderVertexBuff);
    glGenBuffers(1, &imguiSetup.shaderElementsBuff);
    glGenVertexArrays(1, &imguiSetup.shaderVao);

    glBindVertexArray(imguiSetup.shaderVao);
    bindArrayBuffer(imguiSetup.shaderVertexBuff);

    enum Location {
        POSITION = 0,
        UV = 1,
        COLOR = 2
    };

    i32 indexes[] = {POSITION, UV, COLOR};
    enableVertexAttribArrays(indexes, 3);

    vertexAttribPointer(Location::POSITION, 2, GL_FLOAT, false, sizeof(ImDrawVert),
                        (GLvoid*)OFFSETOF(ImDrawVert, pos));
    vertexAttribPointer(Location::UV, 2, GL_FLOAT, false, sizeof(ImDrawVert),
                        (GLvoid*)OFFSETOF(ImDrawVert, uv));
    vertexAttribPointer(Location::COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert),
                        (GLvoid*)OFFSETOF(ImDrawVert, col));

    // TODO: remove this
    constexpr auto mat4Ortho = [](f32* matrix, f32 left, f32 right, f32 top, f32 bottom,
            f32 nearPlane, f32 farPlane)
    {
        memset(matrix, 0, sizeof(f32) * 16);
        matrix[15] = 1.f;

        matrix[0] = 2.f / (right - left);
        matrix[5] = 2.f / (top - bottom);
        matrix[10] = -2.f / (farPlane - nearPlane);
        matrix[12] = -((right + left) / (right - left));
        matrix[13] = -((top + bottom) / (top - bottom));
        matrix[14] = -((farPlane + nearPlane) / (farPlane - nearPlane));
    };

    mat4Ortho(imguiSetup.viewMatrix, 0, io.DisplaySize.x, 0, io.DisplaySize.y, -1, 1);

    OGL_DBG_GROUP_END(SetupImGui);

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glDisable(GL_CULL_FACE);
    //glEnable(GL_DEPTH_TEST);

    //glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef CONF_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, this);
#endif

    initialized = true;
    return true;
}

void frameDoTextureManagement(RendererFrameData& frame)
{
    glDeleteTextures(frame.texToDestroyCount, frame.gpuTexDestroyList);

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
    OGL_DBG_GROUP_BEGIN(SectorRender);

    glUseProgram(shader_tile.program);
    glUniform1i(shader_tile.uDiffuse, 0);
    glUniform1i(shader_tile.uAlphaMask, 1);
    glUniformMatrix4fv(shader_tile.uProjMatrix, 1, GL_FALSE, frame.matCamProj.data);
    glUniformMatrix4fv(shader_tile.uViewMatrix, 1, GL_FALSE, frame.matCamViewIso.data);
    glUniformMatrix4fv(shader_tile.uModelMatrix, 1, GL_FALSE, frame.matSectorTileModel.data);

    bindArrayBuffer(shader_tile.vbo);

    // upload vertex data to gpu
    const i32 tileVertexDataCount = frame.tileVertexData.count();
    const TileVertex* tileVertexData = frame.tileVertexData.data();

    if(frame.doUploadTileVertexData) {
        if(tileVertexDataCount > gpuTileVertexCount) {
            gpuTileVertexCount = max(gpuTileVertexCount * 2, tileVertexDataCount);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * gpuTileVertexCount, 0, GL_DYNAMIC_DRAW);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileVertex) * tileVertexDataCount,
                        tileVertexData);
    }

    glBindVertexArray(shader_tile.vao);

    blendModeOpaque();

    const i32 baseEnd = frame.tvOff_floor;
    for(i32 i = 0; i < baseEnd; i += 6) {
        textureSlot(*frame.tileQuadGpuTex[i/6], 0);
        glDrawArrays(GL_TRIANGLES, i, 6);
    }

    blendModeTransparency();

    const i32 floorEnd = frame.tvOff_mixed;
    const i32 alphaMaskTexOffset = (floorEnd - baseEnd) / 6;
    for(i32 i = baseEnd; i < floorEnd; i += 6) {
        textureSlot(*frame.tileQuadGpuTex[i/6], 0);
        textureSlot(*frame.tileQuadGpuTex[i/6 + alphaMaskTexOffset], 1);
        glDrawArrays(GL_TRIANGLES, i, 6);
    }

    mat4 ident = mat4Identity();
    glUniformMatrix4fv(shader_tile.uViewMatrix, 1, GL_FALSE, frame.matCamViewOrtho.data);
    glUniformMatrix4fv(shader_tile.uModelMatrix, 1, GL_FALSE, ident.data);

    const i32 mixedEnd = tileVertexDataCount;
    for(i32 i = floorEnd; i < mixedEnd; i += 6) {
        textureSlot(*frame.tileQuadGpuTex[alphaMaskTexOffset + i/6], 0);
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

    bindArrayBuffer(shader_dbgColor.vbo_vertexData);
    const i32 dbgQuadVertCount = frame.dbgQuadVertData.count();
    if(dbgQuadVertCount > gpuDbgQuadVertexCount) {
        gpuDbgQuadVertexCount = max(gpuDbgQuadVertexCount * 2, dbgQuadVertCount);
        glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * gpuDbgQuadVertexCount, 0, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(QuadVertex) * dbgQuadVertCount,
                    frame.dbgQuadVertData.data());

    glBindVertexArray(shader_dbgColor.vao);

    blendModeOpaque();

    const mat4* matModels = frame.dbgQuadModelMat.data();
    const i32 meshCount = frame.dbgQuadMeshDef.count();
    const MeshDef* meshDef = frame.dbgQuadMeshDef.data();

    for(i32 i = 0; i < meshCount; ++i) {
        glUniformMatrix4fv(shader_dbgColor.uModelMatrix, 1, GL_FALSE, matModels[i].data);
        glDrawArrays(GL_TRIANGLES, meshDef[i].vertOffset, meshDef[i].vertCount);
    }


    OGL_DBG_GROUP_END(DbgObjects);
}

void frameDoImGui(const RendererFrameData& frame)
{
    ImGuiIO& io = ImGui::GetIO();
    i32 fb_width = (i32)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    i32 fb_height = (i32)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    const i32 imguiDrawListCount = frame.imguiDrawList.count();
    ImDrawList* drawLists = frame.imguiDrawList.data();

    // scale clip rects
    const ImVec2 scale = io.DisplayFramebufferScale;
    for(i32 i = 0; i < imguiDrawListCount; i++) {
        assert(frame.imguiDrawList[i].VtxBuffer.size() > 0);
        assert(drawLists[i].VtxBuffer.size() > 0);

        ImDrawList& cmd_list = drawLists[i];
        for(i32 cmd_i = 0; cmd_i < cmd_list.CmdBuffer.Size; cmd_i++) {
            ImDrawCmd* cmd = &cmd_list.CmdBuffer[cmd_i];
            cmd->ClipRect = ImVec4(cmd->ClipRect.x * scale.x, cmd->ClipRect.y * scale.y,
                                   cmd->ClipRect.z * scale.x, cmd->ClipRect.w * scale.y);
        }
    }

    // Backup GL state
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,
                                                   &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    OGL_DBG_GROUP_BEGIN(IMGUI_RENDER_SETUP);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup viewport, orthographic projection matrix
    const ImGuiGLSetup& ui = imguiSetup;
    glUseProgram(ui.shaderProgram);
    glUniformMatrix4fv(ui.shaderViewUni, 1, GL_FALSE, ui.viewMatrix);
    glUniform1i(ui.shaderTextureUni, 0);

    glBindVertexArray(ui.shaderVao);

    OGL_DBG_GROUP_END(IMGUI_RENDER_SETUP);

    for(i32 n = 0; n < imguiDrawListCount; ++n) {
        const ImDrawList* cmd_list = &drawLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        OGL_DBG_GROUP_BEGIN(IMGUI_RENDER_BUFFER_BINDING);

        glBindBuffer(GL_ARRAY_BUFFER, ui.shaderVertexBuff);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert),
                     (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui.shaderElementsBuff);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx),
                     (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        OGL_DBG_GROUP_END(IMGUI_RENDER_BUFFER_BINDING);

        for(const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin();
            pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
            if(pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w),
                          (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                          (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT,
                               idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

void processFrames()
{
    Window& client = *get_clientWindow();
    while(client.clientRunning) {
        timept t0 = timeNow();
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
        frameTime = timeDuration(timeNow() - t0);
    }
}

void cleanUp()
{
    LOG_DBG("Renderer> cleaning up...");
}

void pushFrame(const RendererFrameData& frameData_)
{
    // prevent important frames from being squashed
	// FIXME: can get stuck
    while(frameData[backFrameId].doUploadTileVertexData || frameData[backFrameId].texToCreateCount > 0
          || frameData[backFrameId].texToDestroyCount > 0) {
        _mm_pause();
    }

    frameDataMutex[backFrameId].lock();
    frameData[backFrameId].copy(frameData_);

    assert(frameData_.imguiDrawList.data() != frameData[backFrameId].imguiDrawList.data());

    for(i32 i = 0; i < frameData_.imguiDrawList.count(); i++) {
        assert(frameData[backFrameId].imguiDrawList[i].VtxBuffer.size() ==
               frameData_.imguiDrawList[i].VtxBuffer.size());
        assert(frameData[backFrameId].imguiDrawList[i].VtxBuffer.Data !=
               frameData_.imguiDrawList[i].VtxBuffer.Data);
        assert(frameData[backFrameId].imguiDrawList[i].VtxBuffer.size() > 0);
    }

    frameReady[backFrameId] = true;
    frameDataMutex[backFrameId].unlock();
}

};

static Renderer* g_rendererPtr = nullptr;

unsigned long thread_renderer(void*)
{
    LOG("thread_renderer started [%x]", threadGetId());

    static Renderer r;
    g_rendererPtr = &r;
    if(!r.init()) {
        return 1;
    }

    r.processFrames();
    r.cleanUp();

    return 0;
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
