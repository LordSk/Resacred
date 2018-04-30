#pragma once
#include "rs_math.h"

void dbgDrawInit();
void dbgDrawSetView(const mat4& proj, const mat4& view);
void dbgDrawSolidSquare(const vec3f& pos, const vec3f& size, const u32 color);
