#pragma once
#include "rs_base.h"
#include "rs_allocator.h"

typedef enum: i32 {
    NO_FILE_ERROR=0,
    CANNOT_FIND,
    CANNOT_OPEN,
    OUT_OF_MEMORY,
} FileError;

struct FileBuffer
{
    MemBlock block = NULL_MEMBLOCK;
    i32 fileSize = 0;
    FileError error = FileError::NO_FILE_ERROR;
};

/**
 * @brief Allocate a buffer containing the contents of the file
 * @param path File path
 * @param out_pFileSize File buffer length
 * @param pAlloc allocator to use (default: malloc)
 * @return FileBuffer
 */
FileBuffer fileReadWhole(const char* path, IAllocator* pAlloc = MEM_CONTEXT.allocator);

/**
 * @brief Copy content of the file to pFileBuff
 * @param path
 * @param pFileBuff
 * @param buffSize
 * @param pFileLength
 * @return success
 */
bool fileReadWholeCopy(const char* path, char* out_pFileBuff, u32 buffSize, i32* out_pFileLength);

/**
 * @brief Write pBuffer to file
 * @param path
 * @param pBuffer
 * @param buffSize
 * @return success
 */
bool fileWriteBuffer(const char* path, const char* pBuffer, u32 buffSize);

// Pak header (size=256)
struct PakHeader
{
    char typeString[3];
    u8 version;
    i32 entryCount;
    u8 _unkown[248];
};

static_assert(sizeof(PakHeader) == 256, "sizeof(PakHeader) != 256");


struct PakTexture
{
    char filename[32];
    u16 width;
    u16 height;
    u8 typeId;
    u32 compressedSize;
    u8 unknown1;
    u32 offset; // set by Sacred after being loaded
    u8 null[39];
    // -- header size = 80
    u8* data;
};

bool pak_tilesRead(const char* filepath, void** tiles);
bool pak_texturesRead(const char* filepath, PakTexture** textures);
