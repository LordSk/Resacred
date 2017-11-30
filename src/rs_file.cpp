#include "rs_file.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

FileBuffer fileReadWhole(const char* path, IAllocator* pAlloc)
{
    FileBuffer fb;
    FILE* file = fopen(path, "rb");
    if(!file) {
        LOG_ERR("fileReadWhole(path=%s) file could not be open", path);
        fb.error = FileError::CANNOT_OPEN;
        return fb;
    }
    defer(fclose(file));

    u32 start = ftell(file);
    fseek(file, 0, SEEK_END);
    u32 len = ftell(file) - start;
    fseek(file, start, SEEK_SET);

    fb.fileSize = len;
    fb.block = pAlloc->alloc(len + 1);
    if(!fb.block.ptr) {
        LOG_ERR("fileReadWhole(path=%s size=%d) out of memory", path, len);
        fb.error = FileError::OUT_OF_MEMORY;
        return fb;
    }
    char* pFileBuff = (char*)fb.block.ptr;

    // read
    fread(pFileBuff, 1, len, file);
    pFileBuff[len] = 0;
    return fb;
}


bool fileReadWholeCopy(const char* path, char* out_pFileBuff, u32 buffSize, i32* out_pFileLength)
{
    // https://msdn.microsoft.com/en-us/library/40bbyw78.aspx
    int fileHandle = _open(path, O_RDONLY | O_BINARY);
    if(fileHandle == -1) {
        return false;
    }
    int bytesRead = _read(fileHandle, out_pFileBuff, buffSize);
    *out_pFileLength = bytesRead;
    _close(fileHandle);
    return true;
}

bool fileWriteBuffer(const char* path, const char* pBuffer, u32 buffSize)
{
    int fileHandle = _open(path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWRITE);
    if(fileHandle == -1) {
        return false;
    }
    int bytesWritten = _write(fileHandle, pBuffer, buffSize);
    _close(fileHandle);
    return bytesWritten != -1;
}

struct PakTile
{
    char filename[32]; // iso%d.tga
    i32 textureId;
    i32 tileId;
    i32 _unknown[6]; // always the same and unrelevant
};

struct SubFileDesc
{
    i32 vint1;
    i32 offset;
    i32 vint2;
};

static_assert(sizeof(PakTile) == 64, "sizeof(Tile) != 64");

bool pak_tilesRead(const char* filepath, void** tiles)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    SubFileDesc* fileDesc = (SubFileDesc*)(top + sizeof(PakHeader));

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTile* tile = (PakTile*)(top + offset);
        /*LOG("[%d] off=0x%x filename=%32s texId=%d tileId=%d %d %d %d %d %d %d", i, offset,
            tile->filename, tile->textureId, tile->tileId,
            tile->_unknown[0], tile->_unknown[1], tile->_unknown[2], tile->_unknown[3], tile->_unknown[4],
            tile->_unknown[5]);*/
    }

    return true;
}

struct TGAHeader
{
    u8 imageLength;
    u8 colorMapType;
    u8 imageType;
    u8 colorMapSpecification[5];

    // image specification
    u16 originX;
    u16 originY;
    u16 width;
    u16 height;
    u8 pixelDepth;
    u8 imageDescriptor;
};

bool pak_texturesRead(const char* filepath, PakTexture** textures)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    //defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    SubFileDesc* fileDesc = (SubFileDesc*)(top + sizeof(PakHeader));

    PakTexture* outTex = (PakTexture*)MEM_ALLOC(sizeof(PakTexture) * entryCount).ptr;
    *textures = outTex;

    for(i32 i = 1; i < entryCount; ++i) { // first one is fucked offset is 0
        i32 offset = fileDesc[i].offset;
        /*LOG("[%d] 1=%d off=0x%x 3=%d", i, fileDesc[i].vint1,
            fileDesc[i].offset, fileDesc[i].vint1);*/

        PakTexture& tex = *(PakTexture*)(top + offset);
        /*LOG("[%d] off=0x%x width=%d height=%d offset=%d ", i, offset,
            tex.width, tex.height, tex.offset);*/
        outTex[i] = tex;
        outTex[i].data = (u8*)(&tex) + 80;

        TGAHeader& tga = *(TGAHeader*)outTex[i].data;
        /*LOG("imageLength=%d colorMapType=%d imageType=%d originX=%d originY=%d width=%d "
            "height=%d pixelDepth=%d imageDescriptor=%x",
            tga.imageLength, tga.colorMapType, tga.imageType, tga.originX, tga.originY,
            tga.width, tga.height, tga.pixelDepth, tga.imageDescriptor);*/
    }

    return true;
}
