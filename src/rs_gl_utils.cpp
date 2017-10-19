#include "rs_gl_utils.h"
#include "rs_allocator.h"
#include "rs_file.h"

GLuint glMakeShader(const GLShaderFile files[], const i32 count)
{
    assert(count < 6);

    GLuint shaders[6];
    for(i32 i = 0; i < count; ++i) {
        shaders[i] = glCreateShader(files[i].type);

        // compile
        glShaderSource(shaders[i], 1, &files[i].buff, &files[i].size);
        glCompileShader(shaders[i]);

        // check result
        GLint compileResult = GL_FALSE;
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &compileResult);

        if(!compileResult) {
            int logLength = 0;
            glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &logLength);
            MemBlock block = MEM_ALLOC(logLength);
            char* logBuff = (char*)block.ptr;
            glGetShaderInfoLog(shaders[i], logLength, NULL, logBuff);
            LOG_ERR("ERROR: could not compile shader (id=%d type=%d msg=%s)", i, files[i].type, logBuff);
            MEM_DEALLOC(block);
            glDeleteShader(shaders[i]);
            return 0;
        }
    }

    GLuint program = glCreateProgram();

    for(u32 i = 0; i < count; ++i) {
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);

    // check
    GLint linkResult = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkResult);

    if(!linkResult) {
        int logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        MemBlock block = MEM_ALLOC(logLength);
        char* logBuff =  (char*)block.ptr;
        glGetProgramInfoLog(program, logLength, NULL, logBuff);
        LOG_ERR("ERROR: could not link program (%s)", logBuff);
        MEM_DEALLOC(block);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint glOpenAndMakeShader(const char* filepaths[], const i32 types[], const i32 count)
{
    assert(count < 6);
    FileBuffer buffs[6];
    GLShaderFile shaderFbs[6];

    for(i32 i = 0; i < count; ++i) {
        buffs[i] = fileReadWhole(filepaths[i]);
        if(!buffs[i].block.ptr) {
            LOG_ERR("ERROR: could not open %s", filepaths[i]);
            return 0;
        }

        shaderFbs[i].buff = (const char*)buffs[i].block.ptr;
        shaderFbs[i].size = buffs[i].fileSize;
        shaderFbs[i].type = types[i];
    }

    return glMakeShader(shaderFbs, count);
}

GLuint glMakeArrayBuffer(const void* data, const u64 size, GLenum usage)
{
    GLuint buffId;
    glGenBuffers(1, &buffId);
    glBindBuffer(GL_ARRAY_BUFFER, buffId);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    return buffId;
}
