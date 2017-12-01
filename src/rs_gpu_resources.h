#pragma once
#include "rs_base.h"

bool GPUres_init(struct DiskTextures* diskTextures);
void GPUres_newFrame();
void GPUres_requestTextures(i32* pakTextureIds, u32** outGpuTexHandles, const i32 requestCount);
void GPUres_deinit();
