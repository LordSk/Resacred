#pragma once
#include "gl3w.h"
#include "rs_base.h"
#include "rs_array.h"
#include "rs_thread.h"

#ifdef CONF_DEBUG
    #define OGL_DBG_GROUP_BEGIN(name) \
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, -(__COUNTER__^(__LINE__*1337)), strLen(#name), #name)
    #define OGL_DBG_GROUP_END(name) glPopDebugGroup()
#else
    #define OGL_DBG_GROUP_BEGIN(name)
    #define OGL_DBG_GROUP_END(name)
#endif

struct RBarrier
{
    String32 name;
    AtomicCounter counter;
    void _create(const char* name_);
    void _release();
    void _wait();
};

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

struct PipelineState
{
    i32 shader;
    i32 cullMode;
    i32 primitiveInterpretation = GL_TRIANGLE_STRIP;
};

struct CommandList
{
    typedef enum {
        CT_CLEAR=0,
        CT_CLEAR_COLOR,                 // 1

        CT_CREATE_SHADER_COMPILE,       // 2
        CT_CREATE_TEXTURE2D,            // 3
        CT_DESTROY_TEXTURE,             // 4
        CT_GET_UNIFORM_LOCATION,        // 5
        CT_ENABLE_VERTEX_ATTRIB_ARRAY,  // 6
        CT_VERTEX_ATTRIB_POINTER,       // 7
        CT_GEN_BUFFERS,                 // 8
        CT_GEN_VERTEX_ARRAYS,           // 9
        CT_BIND_BUFFER,                 // 10
        CT_BIND_VERTEX_ARRAY,           // 11
        CT_ARRAY_BUFFER_DATA,           // 12
        CT_DRAW_TRIANGLES,              // 13
        CT_USE_PROGRAM,                 // 14
        CT_UNIFORM_INT,                 // 15
        CT_UNIFORM_4FV,                 // 16
        CT_UNIFORM_MAT4,                // 17
        CT_TEXTURE_SLOT,                // 18

        CT_QUERY_VRAM_INFO,

        CT_COUNTER_INCREMENT,
        CT_COUNTER_DECREMENT,
        CT_BARRIER,
        CT_END_FRAME,
        CT_EXECUTE,
    } CmdType;

    struct Cmd
    {
        CmdType type;
        void* param[7];
    };
    Array<Cmd,256> cmds;

    inline void clearCommands() {
        cmds._count = 0;
    }

    inline void endFrame() {
        Cmd cmd;
        cmd.type = CT_END_FRAME;
        cmds.pushPOD(&cmd, 1);
    }

    inline void execute() {
        Cmd cmd;
        cmd.type = CT_EXECUTE;
        cmds.pushPOD(&cmd, 1);
    }

    inline void clear() {
        Cmd cmd;
        cmd.type = CT_CLEAR;
        cmds.pushPOD(&cmd, 1);
    }

    inline void clearColor(f32 r, f32 g, f32 b) {
        Cmd cmd;
        cmd.type = CT_CLEAR_COLOR;
        cmd.param[0] = *(void**)&r;
        cmd.param[1] = *(void**)&g;
        cmd.param[2] = *(void**)&b;
        cmds.pushPOD(&cmd, 1);
    }

    inline void barrier(RBarrier* barrier, const char* barrierName = "") {
        barrier->_create(barrierName);
        Cmd cmd;
        cmd.type = CT_BARRIER;
        cmd.param[0] = (void*)barrier;
        cmds.pushPOD(&cmd, 1);
    }

    void createShaderAndCompile(const MemBlock fileBuffers[], const i32 types[], i32 count,
                             GLuint* out_program) {
        Cmd cmd;
        cmd.type = CT_CREATE_SHADER_COMPILE;
        cmd.param[0] = (void*)fileBuffers;
        cmd.param[1] = (void*)types;
        cmd.param[2] = (void*)(intptr_t)count;
        cmd.param[3] = out_program;
        cmds.pushPOD(&cmd, 1);
    }

    inline void createTexture2D(const TextureDesc2D* desc, GLuint* out_texture) {
        Cmd cmd;
        cmd.type = CT_CREATE_TEXTURE2D;
        cmd.param[0] = (void*)desc;
        cmd.param[1] = (void*)out_texture;
        cmds.pushPOD(&cmd, 1);
    }

    inline void destroyTexture(GLuint texture) {
        Cmd cmd;
        cmd.type = CT_DESTROY_TEXTURE;
        cmd.param[0] = (void*)(intptr_t)texture;
        cmds.pushPOD(&cmd, 1);
    }

    inline void enableVertexAttribArrays(i32 indexes[], i32 count) {
        Cmd cmd;
        cmd.type = CT_ENABLE_VERTEX_ATTRIB_ARRAY;
        cmd.param[0] = (void*)indexes;
        cmd.param[1] = (void*)(intptr_t)count;
        cmds.pushPOD(&cmd, 1);
    }

    inline void vertexAttribPointer(i32 index, i32 numComp, i32 type, i32 normalized, i32 stride,
                                    void* pointer) {
        Cmd cmd;
        cmd.type = CT_VERTEX_ATTRIB_POINTER;
        cmd.param[0] = (void*)(intptr_t)index;
        cmd.param[1] = (void*)(intptr_t)numComp;
        cmd.param[2] = (void*)(intptr_t)type;
        cmd.param[3] = (void*)(intptr_t)normalized;
        cmd.param[4] = (void*)(intptr_t)stride;
        cmd.param[5] = pointer;
        cmds.pushPOD(&cmd, 1);
    }

    inline void getUniformLocations(GLuint* program, i32* locations[], const char* uniformNames[],
                                    i32 count) {
        Cmd cmd;
        cmd.type = CT_GET_UNIFORM_LOCATION;
        cmd.param[0] = (void*)program;
        cmd.param[1] = (void*)locations;
        cmd.param[2] = (void*)uniformNames;
        cmd.param[3] = (void*)(intptr_t)count;
        cmds.pushPOD(&cmd, 1);
    }

    inline void genBuffers(GLuint buffers[], i32 count) {
        Cmd cmd;
        cmd.type = CT_GEN_BUFFERS;
        cmd.param[0] = (void*)buffers;
        cmd.param[1] = (void*)(intptr_t)count;
        cmds.pushPOD(&cmd, 1);
    }

    inline void genVertexArrays(GLuint vaos[], i32 count) {
        Cmd cmd;
        cmd.type = CT_GEN_VERTEX_ARRAYS;
        cmd.param[0] = (void*)vaos;
        cmd.param[1] = (void*)(intptr_t)count;
        cmds.pushPOD(&cmd, 1);
    }

    inline void bindArrayBuffer(GLuint* buffer) {
        Cmd cmd;
        cmd.type = CT_BIND_BUFFER;
        cmd.param[0] = (void*)(intptr_t)GL_ARRAY_BUFFER;
        cmd.param[1] = (void*)buffer;
        cmds.pushPOD(&cmd, 1);
    }

    inline void bindVertexArray(GLuint* vao) {
        Cmd cmd;
        cmd.type = CT_BIND_VERTEX_ARRAY;
        cmd.param[0] = (void*)vao;
        cmds.pushPOD(&cmd, 1);
    }

    inline void arrayBufferData(GLuint* buffer, const void* data, i32 dataSize, GLenum usage) {
        Cmd cmd;
        cmd.type = CT_ARRAY_BUFFER_DATA;
        cmd.param[0] = (void*)buffer;
        cmd.param[1] = (void*)data;
        cmd.param[2] = (void*)(intptr_t)dataSize;
        cmd.param[3] = (void*)(intptr_t)usage;
        cmds.pushPOD(&cmd, 1);
    }

    inline void drawTriangles(i32 offset, i32 vertCount) {
        Cmd cmd;
        cmd.type = CT_DRAW_TRIANGLES;
        cmd.param[0] = (void*)(intptr_t)offset;
        cmd.param[1] = (void*)(intptr_t)vertCount;
        cmds.pushPOD(&cmd, 1);
    }

    inline void useProgram(GLuint* program) {
        Cmd cmd;
        cmd.type = CT_USE_PROGRAM;
        cmd.param[0] = (void*)program;
        cmds.pushPOD(&cmd, 1);
    }

    inline void uniformInt(i32 location, i32* value) {
        assert(value);
        Cmd cmd;
        cmd.type = CT_UNIFORM_INT;
        cmd.param[0] = (void*)(intptr_t)location;
        cmd.param[1] = (void*)value;
        cmds.pushPOD(&cmd, 1);
    }

    inline void uniform4fv(i32 location, i32* value) {
        Cmd cmd;
        cmd.type = CT_UNIFORM_4FV;
        cmd.param[0] = (void*)(intptr_t)location;
        cmd.param[1] = (void*)value;
        cmds.pushPOD(&cmd, 1);
    }

    inline void uniformMat4(i32 location, const void* matrix4x4) {
        Cmd cmd;
        cmd.type = CT_UNIFORM_MAT4;
        cmd.param[0] = (void*)(intptr_t)location;
        cmd.param[1] = (void*)matrix4x4;
        cmds.pushPOD(&cmd, 1);
    }

    // TODO: make slot a pointer?
    inline void textureSlot(GLuint* gpuTex, i32 slot) {
        Cmd cmd;
        cmd.type = CT_TEXTURE_SLOT;
        cmd.param[0] = (void*)gpuTex;
        cmd.param[1] = (void*)(intptr_t)slot;
        cmds.pushPOD(&cmd, 1);
    }

    inline void queryVramInfo(i32* dedicated, i32* availMemory, i32* availVidMem,
                              i32* evictionCount, i32* evictedMem) {
        Cmd cmd;
        cmd.type = CT_QUERY_VRAM_INFO;
        cmd.param[0] = dedicated;
        cmd.param[1] = availMemory;
        cmd.param[2] = availVidMem;
        cmd.param[3] = evictionCount;
        cmd.param[4] = evictedMem;
        cmds.pushPOD(&cmd, 1);
    }

    inline void counterIncrement(AtomicCounter* counter) {
        Cmd cmd;
        cmd.type = CT_COUNTER_INCREMENT;
        cmd.param[0] = counter;
        cmds.pushPOD(&cmd, 1);
    }

    inline void counterDecrement(AtomicCounter* counter) {
        Cmd cmd;
        cmd.type = CT_COUNTER_DECREMENT;
        cmd.param[0] = counter;
        cmds.pushPOD(&cmd, 1);
    }
};

i32 thread_renderer(void* data);

void renderer_pushCommandList(const CommandList& list);
void renderer_waitForBarrier(RBarrier* barrier);
void renderer_waitForInit();

