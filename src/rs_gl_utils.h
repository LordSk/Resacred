#pragma once
#include "gl3w.h"
#include "rs_base.h"

struct GLShaderFile
{
    const char* buff;
    i32 size;
    i32 type;
};

GLuint glMakeShader(const GLShaderFile files[], const i32 count);
GLuint glOpenAndMakeShader(const char* filepaths[], const i32 types[], const i32 count);

GLuint glMakeArrayBuffer(const void* data, const u64 size, GLenum usage);

inline void glBindActiveTex(i32 type, i32 slot, GLuint texture)
{
    glActiveTexture(slot);
    glBindTexture(type, texture);
}
