#pragma once
#include "rs_base.h"
#include "rs_allocator.h"
#include "rs_array.h"

unsigned long thread_fileIO(void*);

enum class FileError: i32 {
    NO_FILE_ERROR=0,
    CANNOT_FIND,
    CANNOT_OPEN,
    OUT_OF_MEMORY,
};

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

/**
 * Syncronous File IO
 */

bool fileOpenToRead(const char* path, DiskFile* file);
void fileClose(DiskFile* file);
void fileReadFromPos(const DiskFile* file, i64 from, i64 size, void* dest);
void fileReadAdvance(DiskFile* file, i64 size, void* dest);


/**
 * Asyncronous File IO
 *
 * - all calls are asynchronous
 * - counter is DECREMENTED when request is completed
 * - internally calls to synchronous functions
 */

struct AsyncFileRequest
{
    i32 requestUID;
};

AsyncFileRequest fileAsyncReadAbsolute(const DiskFile* file, i64 start, i64 size, u8* out, AtomicCounter* counter);



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
    i32 type;
    i32 offset;
    i32 size;
};

static_assert(sizeof(PakHeader) == 256, "sizeof(PakHeader) != 256");


#pragma pack(1)
struct PakTextureHeader
{
    char filename[32];
    u16 width;
    u16 height;
    u8 typeId;
    u32 compressedSize;
    u8 unknown1;
    u32 offset; // set by Sacred after being loaded
    u8 null[34];
};
#pragma pack()

static_assert(sizeof(PakTextureHeader) == 80, "sizeof(PakTextureHeader) != 80");

enum class PakTextureType: u16 {
    TYPE_RGBA8 = 6,
    TYPE_ARGB4 = 4
};

struct KeyxSector
{
    char name32[32];
    i32 int0;
    i32 sectorId;
//40
    i32 beforeNbs;
    u16 neighbourIds[8];
//60
    i32 posX1;
    i32 posY1;
    i32 posX2;
    i32 posY2;

    struct {
        i32 int0;
        i32 fileOffset;
        i32 size;
    } subs[32];

    u8 data[308];
};

static_assert(sizeof(KeyxSector) == 0x300, "sizeof(KeyxSector) != 0x300");

struct WldxEntry
{
    i32 tileId;
    i32 staticId;
    i32 entityId;
    i32 floorId;
    u8 rest[8];
    i8 smthX; // probably color related
    i8 smthY;
    i8 smthZ;
    i8 smthW;
    u8 offsetX;
    u8 offsetY;
    u8 flags;
    u8 someTypeId; // 0-15
};

static_assert(sizeof(WldxEntry) == 32, "sizeof(WldxEntry) != 32");

struct SectorxhHeightEntry
{
    i32 unk[4];
    i32 offsetData;
    u8 unk2[16];
};

static_assert(sizeof(SectorxhHeightEntry) == 36, "sizeof(SectorxhHeightData) != 36");

struct SectorxData
{
    char name[32];
    WldxEntry data[4096];
    SectorxhHeightEntry heightData[1];
};

struct DiskTextures
{
    struct TexName {
        char data[32];
    };
    struct TexInfo {
        u16 width;
        u16 height;
        PakTextureType type;
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

    Array<Sector,6051> sectors;
    WldxEntry* entryData;
    MemBlock block;
};

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

struct PakTile
{
    char filename[32]; // iso%d.tga
    i32 textureId;
    i32 tileId;
    i32 _unknown[6]; // always the same and unrelevant
};

static_assert(sizeof(PakTile) == 64, "sizeof(Tile) != 64");

struct FloorEntry
{
    i32 id;
    u32 pakTileIds;
    i32 varC;
    i32 nextFloorId;
};

static_assert(sizeof(FloorEntry) == 16, "sizeof(FloorEntry) != 16");

bool pak_tilesRead(const char* filepath, DiskTiles* diskTiles);
//bool pak_texturesRead(const char* filepath, DiskTextures* textures);
bool bin_WorldRead(const char* filepath);
bool pak_FloorRead(const char* filepath);
bool keyx_sectorsRead(const char* keyx_filepath, const char* wldx_filepath, DiskSectors* diskSectors);
void deflateSectorData(DiskFile* file, i32 offset, i32 compSize, i32 uncompSize, u8* outFileBuffer, u8* outData);

bool pak_textureRead(char* fileBuff, i64 size, i32* out_width, i32* out_height, i32* out_type,
                     u8* out_data, i32* out_size, char* out_name);


struct PakMixedDesc
{
    i32 count;
    i16 width;
    i16 height;
    i16 offX;
    i16 offY;
    i32 mixedDataId;
};

static_assert(sizeof(PakMixedDesc) == 16, "sizeof(PakMixedDesc) != 16");

struct PakMixedData
{
    i32 textureId;
    u16 width;
    u16 height;
    u16 x;
    u16 y;
    u16 zero[2]; // probably padding
    f32 uvX1;
    f32 uvY1;
    f32 uvX2;
    f32 uvY2;
};

static_assert(sizeof(PakMixedData) == 32, "sizeof(PakMixedData) != 32");

struct PakMixedEntry
{
    char name[32];
    PakMixedData data;
};

static_assert(sizeof(PakMixedEntry) == 64, "sizeof(PakMixedEntry) != 64");

struct PakMixedFileData
{
    PakMixedDesc* desc;
    PakMixedData* mixed;
    i32 descCount = 0;
    i32 mixedCount = 0;
    MemBlock block;
};

bool pak_mixedRead(PakMixedFileData* out);

#pragma pack(1)
struct PakStatic
{
    i32 id;
    i32 itemTypeId;
    i32 field_8;
    i16 field_C;
    i32 field_E;
    i32 field_12;
    u8 unk_0;
    i32 parentId;
    i32 anotherParentId;
    i32 nextStaticId;
    i16 patchX;
    i16 patchY;
    i32 triggerId;
    i8 field_2B;
    i8 field_2C;
    u8 layer;
    i8 smthX;
    i8 smthY;
    i8 smthZ;
    i8 field_31;
    i8 field_32;
    i8 field_33;
    u8 unk_1;
    i8 field_35;
    i8 unk_2[10];
};
#pragma pack()

//MESSAGE_SIZEOF(PakStatic);
static_assert(sizeof(PakStatic) == 64, "sizeof(PakStatic) != 64");

bool pak_staticRead(const char* path, Array<PakStatic>* out);

#pragma pack(1)
struct PakItemType
{
    i32 flags;
    i32 int_1;
    i32 itemTextureId;
    i32 field_C;
    i32 mixedId;
    i32 field_14;
    i32 spawnInfoId;
    i32 field_1C;
    i32 field_20;
    i32 soundProfileId;
    u8 unk[15];
    char grnName[32];
    u8 unk2[41];
};
#pragma pack()

//MESSAGE_SIZEOF(PakItemType);
static_assert(sizeof(PakItemType) == 128, "sizeof(PakItemType) != 128");

bool pak_itemRead(const char* path, Array<PakItemType>* out);
