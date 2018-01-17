#include "rs_disk_resources.h"
#include "rs_file.h"

static void diskResource_loadTexture(char* buff, i64 size);

struct DiskResources {

DiskFile fileTexture;
i32 textureCount;
i32* textureOffset;
MemBlock textureOffsetBlock;

bool init()
{
    if(!fileOpenToRead("../sacred_data/texture.pak", &fileTexture)) {
        return false;
    }

    PakHeader header;
    fileReadAdvance(&fileTexture, sizeof(header), &header);
    textureCount = header.entryCount;
    textureOffsetBlock = MEM_ALLOC(sizeof(i32) * textureCount);
    assert(textureOffsetBlock.ptr);
    textureOffset = (i32*)textureOffsetBlock.ptr;

    i64 fileOffsetsSize = textureCount * sizeof(PakSubFileDesc);
    MemBlock b = MEM_ALLOC(fileOffsetsSize);
    assert(b.ptr);

    PakSubFileDesc* fileOffsets = (PakSubFileDesc*)b.ptr;
    fileReadAdvance(&fileTexture, fileOffsetsSize, fileOffsets);

    for(i32 i = 0; i < textureCount; ++i) {
        textureOffset[i] = fileOffsets[i].offset;
    }

    MEM_DEALLOC(b);

    return true;
}

void deinit()
{
    MEM_DEALLOC(textureOffsetBlock);
    fileClose(&fileTexture);
}

void requestTextures(const i32* textureIds, const i32 textureCount)
{
    for(i32 i = 0; i < textureCount; ++i) {
        fileAsyncReadAbsolute(&fileTexture, 0, 0, [](char* buff, i64 size, MemBlock block) {
            diskResource_loadTexture(buff, size);
            MEM_DEALLOC(block);
        });
    }
}

void loadTexture(char* buff, i64 size)
{

}

};

static DiskResources DR;

bool diskResource_init()
{
    return DR.init();
}

void diskResource_deinit()
{
    DR.deinit();
}

void diskResource_requestTextures(const i32* textureIds, const i32 textureCount)
{
    DR.requestTextures(textureIds, textureCount);
}

static void diskResource_loadTexture(char* buff, i64 size)
{
    DR.loadTexture(buff, size);
}
