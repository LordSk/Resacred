#pragma once
#include "rs_base.h"

bool diskResource_init();
void diskResource_deinit();

void diskResource_requestTextures(const i32* textureIds, const i32 textureCount);
