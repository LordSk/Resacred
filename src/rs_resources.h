#pragma once
#include "rs_base.h"

bool resource_init();
void resource_deinit();
void resource_newFrame();

void resource_requestTextures(const i32* textureIds, const i32 textureCount);
void resource_requestGpuTextures(const i32* textureUIDs, u32** out_gpuHandles, const i32 textureCount);
u32 resource_defaultGpuTexture();
