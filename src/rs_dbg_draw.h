#pragma once
#include "rs_math.h"

enum class DbgCoordSpace:i32 {
    WORLD = 0,
    SCREEN,
    COUNT
};

void dbgDrawInit();
void dbgDrawSetView(const mat4& proj, const mat4& view, DbgCoordSpace coordSpace);
void dbgDrawSolidSquare(const vec3& pos, const vec3& size, const u32 color,
                        DbgCoordSpace coordSpace = DbgCoordSpace::SCREEN);
void dbgDrawSetFrameData(struct RendererFrameData* frameData);
