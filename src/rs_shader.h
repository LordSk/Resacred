#pragma once
#include "rs_base.h"

namespace shader
{

struct Textured
{
    i32 program;
    i32 uDiffuse;

    void setDiffuse(i32 texId);
};

bool loadShaders();
bool unloadShaders();

}
