#include "rs_renderer.h"
#include "rs_window.h"
#include "rs_logger.h"
#include "rs_gl_utils.h"
#include "gl3w.h"
#include "imgui.h"
#include "imgui_sdl2_setup.h"
#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>

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

#define FRAME_COUNT 3

struct Renderer
{
    SDL_GLContext glContext;
    SDL_mutex* cmdListMutex[FRAME_COUNT];
    CommandList::Cmd cmdList[FRAME_COUNT][2048];
    i32 cmdListCount[FRAME_COUNT];
    u8 frameReady[FRAME_COUNT];

    i32 frameRenderId = 0;
    i32 framePrepareId = 0;

    bool initialized = false;

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

        for(i32 i = 0; i < FRAME_COUNT; ++i) {
            frameReady[i] = false;
            cmdListCount[i] = 0;
            cmdListMutex[i] = SDL_CreateMutex();
        }

        SDL_GL_SetSwapInterval(0); // no vsync

        frameRenderId = 0;
        framePrepareId = 0;

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

        LOG("Renderer> initialized.");
        initialized = true;
        return true;
    }

    void handleQueue()
    {
        Window& client = *get_clientWindow();
        while(1) {
            if(!frameReady[frameRenderId]) {
                continue;
            }

            CommandList::Cmd* _cmdList = cmdList[frameRenderId];
            SDL_mutex* _cmdListMutex = cmdListMutex[frameRenderId];
            i32& _cmdListCount = cmdListCount[frameRenderId];

            SDL_LockMutex(_cmdListMutex);

            for(i32 i = 0; i < _cmdListCount; ++i) {
                CommandList::Cmd& cmd = _cmdList[i];
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
                        glBindBuffer(type, buffer);
                        break; }

                    case CommandList::CT_BIND_VERTEX_ARRAY: {
                        GLuint vao = *(GLuint*)cmd.param[0];
                        glBindVertexArray(vao);
                        break; }

                    case CommandList::CT_BARRIER: {
                        RBarrier* barrier = (RBarrier*)cmd.param[0];
                        barrier->_release();
                        //LOG_DBG("barrier(%s) released", barrier->name);
                        break; }

                    case CommandList::CT_END_FRAME:
                        client.dbguiRender();

                        client.swapBuffers();
                        frameReady[frameRenderId] = false;
                        frameRenderId = (frameRenderId + 1) % FRAME_COUNT;
                        break;

                    case CommandList::CT_EXECUTE:
                        frameReady[frameRenderId] = false;
                        frameRenderId = (frameRenderId + 1) % FRAME_COUNT;
                        break;
                }
            }

            _cmdListCount = 0;
            SDL_UnlockMutex(_cmdListMutex);
        }
    }
};

Renderer* g_rendererPtr = nullptr;

i32 thread_renderer(void* data)
{
    Renderer renderer;
    g_rendererPtr = &renderer;

    if(!renderer.init()) {
        return 1;
    }

    renderer.handleQueue();

    return 0;
}

void renderer_pushCommandList(const CommandList& list)
{
    while(!g_rendererPtr || !g_rendererPtr->initialized) { // TODO: remove this
        return;
    }

    Renderer& rdr = *g_rendererPtr;
    CommandList::Cmd* cmdList = rdr.cmdList[rdr.framePrepareId];
    SDL_mutex* cmdListMutex = rdr.cmdListMutex[rdr.framePrepareId];
    i32& cmdListCount = rdr.cmdListCount[rdr.framePrepareId];

    assert(cmdListCount + list.cmds.count() < 2048);

    SDL_LockMutex(cmdListMutex);
    memmove(cmdList + cmdListCount, list.cmds.data(), sizeof(CommandList::Cmd) * list.cmds.count());

    const i32 newCount = cmdListCount + list.cmds.count();
    for(i32 i = cmdListCount; i < newCount; ++i) {
        if(cmdList[i].type == CommandList::CT_END_FRAME || cmdList[i].type == CommandList::CT_EXECUTE) {
            rdr.frameReady[rdr.framePrepareId] = true;
            rdr.framePrepareId = (rdr.framePrepareId + 1) % FRAME_COUNT;
            break;
        }
    }

    cmdListCount += list.cmds.count();
    SDL_UnlockMutex(cmdListMutex);
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
