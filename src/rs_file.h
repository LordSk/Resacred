#pragma once
#include "rs_base.h"
#include "rs_allocator.h"
#include "rs_array.h"

i32 thread_fileIO(void*);

typedef enum: i32 {
    NO_FILE_ERROR=0,
    CANNOT_FIND,
    CANNOT_OPEN,
    OUT_OF_MEMORY,
} FileError;

struct FileBuffer
{
    MemBlock block = NULL_MEMBLOCK;
    char* buffer = nullptr;
    i32 size = 0;
    FileError error = FileError::NO_FILE_ERROR;
};

#ifdef CONF_WINDOWS
struct Win32_DiskFile
{
    char path[256];
    HANDLE handle = 0;
    i64 size = 0;
    i64 cursor = 0;
};

typedef Win32_DiskFile DiskFile;
#endif

struct AsyncFileRequest
{
    i32 requestUID;
};

typedef void (*Callback_ReqReadAbsolute)(char* buff, i64 size, MemBlock block);

bool fileOpenToRead(const char* path, DiskFile* file);
void fileClose(DiskFile* file);
void fileReadFromPos(const DiskFile* file, i64 from, i64 size, void* dest);
void fileReadAdvance(DiskFile* file, i64 size, void* dest);

AsyncFileRequest fileAsyncReadAbsolute(const DiskFile* file, u8* start, i64 size,
                                       Callback_ReqReadAbsolute callback = nullptr);

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

struct PakSubFileDesc
{
    i32 vint1;
    i32 offset;
    i32 vint2;
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

struct DiskTextures
{
    typedef enum: u16 {
        TYPE_RGBA8 = 6,
        TYPE_ARGB4 = 4
    } TexType;

    struct TexName {
        char data[32];
    };
    struct TexInfo {
        u16 width;
        u16 height;
        TexType type;
    };

    u8** textureData;
    TexName* textureName;
    TexInfo* textureInfo;
    u8* totalPixelData;
    MemBlock block;
    i32 textureCount = 0;
};

struct DiskSectors
{
    struct WldxEntry
    {
        i32 tileId;
        i32 staticId;
        i32 entityId;
        i32 posInfo;
        u8 rest[8];
        i8 smthX; // probably color related
        i8 smthY;
        i8 smthZ;
        i8 smthW;
        u8 unk1;
        u8 unk2;
        u8 unk3;
        u8 someTypeId; // 0-15
    };

    struct Sector
    {
        i32 id;
        i32 posX1;
        i32 posX2;
        i32 posY1;
        i32 posY2;
        WldxEntry* wldxEntries;
        i32 wldxEntryCount;
    };

    Array<Sector,6050> sectors;
    WldxEntry* entryData;
    MemBlock block;
};

static_assert(sizeof(DiskSectors::WldxEntry) == 32, "sizeof(WldxEntry) != 32");

struct DiskTiles
{
    struct Tile
    {
        i16 textureId;
        u8 localId;
        u8 _pad;
    };

    Tile* tiles;
    i32 tileCount;
    MemBlock block;
};

bool pak_tilesRead(const char* filepath, DiskTiles* diskTiles);
bool pak_texturesRead(const char* filepath, DiskTextures* textures);
bool bin_WorldRead(const char* filepath);
bool pak_FloorRead(const char* filepath);
bool keyx_sectorsRead(const char* keyx_filepath, const char* wldx_filepath, DiskSectors* diskSectors);
