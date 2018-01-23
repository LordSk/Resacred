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

void RBarrier::_create(const char* name_)
{
    name = name_;
    counter._count = 1;
}

void RBarrier::_release()
{
    counter.decrement();
}

void RBarrier::_wait()
{
    while(counter.get() != 0) {
        _mm_pause();
    }
}

// defined futher down
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length, const GLchar* message,
                            const void* userParam);

#define QUEUE_COUNT 2
#define QUEUE_LIST_MAX 4096

struct Renderer {

SDL_GLContext glContext;
Array<CommandList::Cmd,QUEUE_LIST_MAX> cmdList[QUEUE_COUNT];
i32 fillingQueueId = 0;
f64 frameTime = 0;

bool initialized = false;
Mutex queueMutex;

PipelineState pipelineState;

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

    for(i32 i = 0; i < QUEUE_COUNT; ++i) {
        cmdList[i].allocator = nullptr;
    }

    SDL_GL_SetSwapInterval(0); // no vsync

    fillingQueueId = 0;

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glDisable(GL_CULL_FACE);

#ifdef CONF_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, this);
#endif

    initialized = true;
    return true;
}

void handleQueue()
{
    Window& client = *get_clientWindow();
    while(client.rdrRunning) {
        auto t0 = timeNow();

        if(cmdList[fillingQueueId].count() == 0) {
            continue;
        }

        // swap queues
        queueMutex.lock();
        const i32 backQueueId = fillingQueueId;
        fillingQueueId ^= 1;
        queueMutex.unlock();

        // execute commands
        CommandList::Cmd* _cmdList = cmdList[backQueueId].data();
        const i32 _cmdListCount = cmdList[backQueueId].count();

        OGL_DBG_GROUP_BEGIN(RENDERER_COMMAND_EXEC);

        for(i32 c = 0; c < _cmdListCount; ++c) {
            CommandList::Cmd& cmd = _cmdList[c];

            switch(cmd.type) {
                case CommandList::CT_CLEAR:
                    glClear(GL_COLOR_BUFFER_BIT);
                    break;

                case CommandList::CT_CLEAR_COLOR:
                    glClearColor(*(GLfloat*)&cmd.param[0],
                                 *(GLfloat*)&cmd.param[1],
                                 *(GLfloat*)&cmd.param[2],
                                 1.f);
                    break;

                case CommandList::CT_CREATE_SHADER_COMPILE: {
                    const MemBlock* fileBuffers = (const MemBlock*)cmd.param[0];
                    const i32* types = (const i32*)cmd.param[1];
                    const i32 count = (i32)(intptr_t)cmd.param[2];
                    GLuint* out_program = (GLuint*)cmd.param[3];

                    /*LOG_DBG("CT_SHADER_CREATE_COMPILE: fileBuffers=%#x types=%#x count=%d"
                            " out_program=%#x",
                            fileBuffers, types, count , out_program);*/

                    GLShaderFile shaderFbs[6];

                    for(i32 i = 0; i < count; ++i) {
                        shaderFbs[i].buff = (const char*)fileBuffers[i].ptr;
                        shaderFbs[i].size = fileBuffers[i].size;
                        shaderFbs[i].type = types[i];
                    }

                    *out_program = glMakeShader(shaderFbs, count);
                    break; }

                case CommandList::CT_CREATE_TEXTURE2D: {
                    const TextureDesc2D& desc = *(const TextureDesc2D*)cmd.param[0];
                    GLuint* out_texture = (GLuint*)cmd.param[1];

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

                    *out_texture = texture;
                    //LOG("Renderer> created texture %d", texture);

                    break; }

                case CommandList::CT_DESTROY_TEXTURE: {
                    GLuint texture = (GLuint)(intptr_t)cmd.param[0];
                    glDeleteTextures(1, &texture);
                    //LOG("Renderer> destroyed texture %d", texture);

                    break; }

                case CommandList::CT_ENABLE_VERTEX_ATTRIB_ARRAY: {
                    const i32* indexes = (const i32*)cmd.param[0];
                    const i32 count = (i32)(intptr_t)cmd.param[1];

                    for(i32 i = 0; i < count; ++i) {
                        glEnableVertexAttribArray(indexes[i]);
                    }

                    break; }

                case CommandList::CT_VERTEX_ATTRIB_POINTER: {
                    i32 index = (i32)(intptr_t)cmd.param[0];
                    i32 numComp = (i32)(intptr_t)cmd.param[1];
                    i32 type = (i32)(intptr_t)cmd.param[2];
                    i32 normalized = (i32)(intptr_t)cmd.param[3];
                    i32 stride = (i32)(intptr_t)cmd.param[4];
                    void* pointer = cmd.param[5];

                    glVertexAttribPointer(
                                index,
                                numComp,
                                type,
                                normalized,
                                stride,
                                pointer);

                    break; }

                case CommandList::CT_GET_UNIFORM_LOCATION: {
                    GLuint program = *(i32*)cmd.param[0];
                    i32** locations = (i32**)cmd.param[1];
                    const char** unifromNames = (const char**)cmd.param[2];
                    i32 count = (i32)(intptr_t)cmd.param[3];

                    for(i32 i = 0; i < count; ++i) {
                        *locations[i] = glGetUniformLocation(program, unifromNames[i]);
                    }

                    break; }

                case CommandList::CT_GEN_BUFFERS: {
                    GLuint* buffers = (GLuint*)cmd.param[0];
                    i32 count = (i32)(intptr_t)cmd.param[1];
                    assert(buffers);
                    glGenBuffers(count, buffers);
                    break; }

                case CommandList::CT_GEN_VERTEX_ARRAYS: {
                    GLuint* vaos = (GLuint*)cmd.param[0];
                    i32 count = (i32)(intptr_t)cmd.param[1];
                    glGenVertexArrays(count, vaos);
                    break; }

                case CommandList::CT_BIND_BUFFER: {
                    i32 type = (i32)(intptr_t)cmd.param[0];
                    GLuint buffer = *(GLuint*)cmd.param[1];
                    assert(buffer != 0);
                    glBindBuffer(type, buffer);
                    break; }

                case CommandList::CT_BIND_VERTEX_ARRAY: {
                    GLuint vao = *(GLuint*)cmd.param[0];
                    glBindVertexArray(vao);
                    break; }

                case CommandList::CT_ARRAY_BUFFER_DATA: {
                    GLuint buffer = *(GLuint*)cmd.param[0];
                    void* data = cmd.param[1];
                    i32 dataSize = (i32)(intptr_t)cmd.param[2];
                    i32 usage = (i32)(intptr_t)cmd.param[3];

                    /*LOG_DBG("CT_ARRAY_BUFFER_DATA> buffer=%d data=%x dataSize=%d",
                            buffer, data, dataSize);*/

                    assert(buffer != 0);
                    glBindBuffer(GL_ARRAY_BUFFER, buffer);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, data, usage);
                    break; }

                case CommandList::CT_BUFFER_SUB_DATA: {
                    GLenum type = (GLenum)(intptr_t)cmd.param[0];
                    GLuint buffer = *(GLuint*)cmd.param[1];
                    i32 offset = (i32)(intptr_t)cmd.param[2];
                    void* data = cmd.param[3];
                    i32 dataSize = (i32)(intptr_t)cmd.param[4];

                    /*LOG_DBG("CT_BUFFER_SUB_DATA> type=%d buffer=%d offset=%d data=%x dataSize=%d",
                            type, buffer, offset, data, dataSize);*/

                    assert(buffer != 0);
                    glBindBuffer(type, buffer);
                    glBufferSubData(type, offset, dataSize, data);
                    break; }

                case CommandList::CT_DRAW_TRIANGLES: {
                    i32 offset = (i32)(intptr_t)cmd.param[0];
                    i32 vertCount = (i32)(intptr_t)cmd.param[1];
                    glDrawArrays(GL_TRIANGLES, offset, vertCount);
                    break; }

                case CommandList::CT_USE_PROGRAM: {
                    GLuint program = *(GLuint*)cmd.param[0];
                    glUseProgram(program);
                    break; }

                case CommandList::CT_UNIFORM_INT: {
                    i32 location = (i32)(intptr_t)cmd.param[0];
                    i32 value = *(i32*)cmd.param[1];
                    glUniform1i(location, value);
                    break; }

                case CommandList::CT_UNIFORM_4FV: {
                    assert(false);
                    break; }

                case CommandList::CT_UNIFORM_MAT4: {
                    i32 location = (i32)(intptr_t)cmd.param[0];
                    f32* matrixData = (f32*)cmd.param[1];
                    glUniformMatrix4fv(location, 1, GL_FALSE, matrixData);
                    break; }

                case CommandList::CT_TEXTURE_SLOT: {
                    GLuint textureId = *(GLuint*)cmd.param[0];
                    i32 slot = (i32)(intptr_t)cmd.param[1];
                    glActiveTexture(GL_TEXTURE0 + slot);
                    glBindTexture(GL_TEXTURE_2D, textureId);
                    break; }

                case CommandList::CT_QUERY_VRAM_INFO: {
                    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, (GLint*)cmd.param[0]);
                    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, (GLint*)cmd.param[1]);
                    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, (GLint*)cmd.param[2]);
                    glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, (GLint*)cmd.param[3]);
                    glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, (GLint*)cmd.param[4]);

                    //LOG_DBG("Renderer> CT_QUERY_VRAM_INFO done");

                    break; }

                case CommandList::CT_LOCK: {
                    MutexSpin* mutex = (MutexSpin*)cmd.param[0];
                    mutex->lock();
                    break; }

                case CommandList::CT_UNLOCK: {
                    MutexSpin* mutex = (MutexSpin*)cmd.param[0];
                    mutex->unlock();
                    break; }

                case CommandList::CT_COUNTER_INCREMENT: {
                    AtomicCounter* counter = (AtomicCounter*)cmd.param[0];
                    counter->increment();
                    break; }

                case CommandList::CT_COUNTER_DECREMENT: {
                    AtomicCounter* counter = (AtomicCounter*)cmd.param[0];
                    counter->decrement();
                    break; }

                case CommandList::CT_BARRIER: {
                    RBarrier* barrier = (RBarrier*)cmd.param[0];
                    barrier->_release();
                    //LOG_DBG("Renderer> barrier(%s) released", barrier->name);
                    break; }

                case CommandList::CT_END_FRAME: {
                    OGL_DBG_GROUP_BEGIN(IMGUI_GROUP);

                    client.dbguiRender();

                    OGL_DBG_GROUP_END(IMGUI_GROUP);

                    client.swapBuffers();
                    /*frameReady[frameRenderId] = false;
                    frameRenderId = (frameRenderId + 1) % FRAME_COUNT;*/
                    //LOG_DBG("Renderer> CT_END_FRAME");
                    break; }

                case CommandList::CT_EXECUTE:
                    /*frameReady[frameRenderId] = false;
                    frameRenderId = (frameRenderId + 1) % FRAME_COUNT;*/
                    //LOG_DBG("Renderer> CT_EXECUTE");
                    break;
                default:
                    assert(0);
                    break;
            }
        }

        OGL_DBG_GROUP_END(RENDERER_COMMAND_EXEC);

        cmdList[backQueueId].clearPOD();
        frameTime = timeDurSince(t0);
    }
}

void pushCommandList(const CommandList& list)
{
    assert(list.cmds.count() < QUEUE_LIST_MAX);

    while(cmdList[fillingQueueId].count() + list.cmds.count() >= (QUEUE_LIST_MAX-1)) {
        _mm_pause(); // Actually very important?
    }

    queueMutex.lock();
    cmdList[fillingQueueId].pushPOD(list.cmds.data(), list.cmds.count());
    queueMutex.unlock();
}

void cleanUp()
{
    LOG_DBG("Renderer> cleaning up...");
}

};

Renderer* g_rendererPtr = nullptr;

unsigned long thread_renderer(void*)
{
    LOG("thread_renderer started [%x]", threadGetId());

    Renderer renderer;
    g_rendererPtr = &renderer;

    if(!renderer.init()) {
        return 1;
    }

    LOG_SUCC("Renderer> initialized");

    renderer.handleQueue();
    renderer.cleanUp();
    g_rendererPtr = nullptr;

    return 0;
}

void renderer_pushCommandList(CommandList& list)
{
    if(list.cmds.count() > 0) {
        g_rendererPtr->pushCommandList(list);
        list.cmds.clearPOD();
    }
}

void renderer_waitForBarrier(RBarrier* barrier)
{
    barrier->_wait();
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
