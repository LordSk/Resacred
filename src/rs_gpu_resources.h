#pragma once
#include "rs_base.h"

/*
 * GPU resource manager
 *
 * Usage:
 *  - request textures every frame
 *  - get a pointer back, dereference to get actual gpu handle
 *  - points to zero while texture is being uploaded to gpu
 *
 *  - textures not being requested will be destroyed
 */


bool GPUres_init(struct DiskTextures* diskTextures);
void GPUres_newFrame();
void GPUres_requestTextures(i32* pakTextureIds, u32** outGpuTexHandles, const i32 requestCount);
void GPUres_deinit();
void GPUres_debugUi();

u32 gpuRes_defaultTexture();
