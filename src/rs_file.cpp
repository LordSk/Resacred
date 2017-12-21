#include "rs_file.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include "zlib.h"

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

bool pak_tilesRead(const char* filepath, DiskTiles* diskTiles)
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

    diskTiles->block = MEM_ALLOC(entryCount * sizeof(DiskTiles::Tile));
    assert(diskTiles->block.ptr);
    diskTiles->tiles = (DiskTiles::Tile*)diskTiles->block.ptr;
    diskTiles->tileCount = entryCount;

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTile* tile = (PakTile*)(top + offset);
        /*LOG("[%d] off=0x%x filename=%32s texId=%d tileId=%d %d %d %d %d %d %d", i, offset,
            tile->filename, tile->textureId, tile->tileId,
            tile->_unknown[0], tile->_unknown[1], tile->_unknown[2], tile->_unknown[3], tile->_unknown[4],
            tile->_unknown[5]);*/
        diskTiles->tiles[i].textureId = tile->textureId;
        diskTiles->tiles[i].localId = tile->tileId;
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

i32 zlib_decompress(const char* input, const i32 inputSize, u8* output, const i32 outputMaxSize,
                    i32* outputSize = nullptr)
{
    constexpr i32 CHUNK = 16384;
    i32 ret;
    u32 have;
    z_stream strm;
    u8 in[CHUNK];
    u8 out[CHUNK];
    i32 inCursor = 0;
    i32 outCursor = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if(ret != Z_OK) {
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        i32 copySize = CHUNK;
        if(inputSize > 0 && inputSize - inCursor < CHUNK) {
            copySize = inputSize - inCursor;
        }
        strm.avail_in = copySize;
        if(copySize <= 0) {
            break;
        }
        memmove(in, input + inCursor, copySize);
        inCursor += copySize;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            memmove(output + outCursor, out, have);
            outCursor += have;
            if(outCursor > outputMaxSize) {
                inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while(strm.avail_out == 0);
    } while(ret != Z_STREAM_END);

    /* clean up and return */
    inflateEnd(&strm);
    if(outputSize) *outputSize = outCursor;
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

bool pak_texturesRead(const char* filepath, DiskTextures* textures)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = min(header->entryCount, 6000);
    SubFileDesc* fileDesc = (SubFileDesc*)(top + sizeof(PakHeader));

    fileDesc++; // skip first (invalid)

    u64 blockSize = entryCount * (sizeof(void*) + sizeof(DiskTextures::TexName) +
                                  sizeof(DiskTextures::TexInfo));

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTexture& tex = *(PakTexture*)(top + offset);

        // 32 bits
        if(tex.typeId == 6) {
            blockSize += tex.width * tex.height * 4;
        }
        // 16 bits
        else {
            blockSize += tex.width * tex.height * 2;
        }
    }

    LOG("pak_texturesRead> entryCount=%d block_size=%llumb", entryCount, blockSize/1024/1024);

    // allocate data block
    textures->block = MEM_ALLOC(blockSize, 4);
    assert(textures->block.ptr);
    u8* cursor = (u8*)textures->block.ptr;
    textures->textureCount = entryCount;
    textures->textureData = (u8**)cursor;
    cursor += sizeof(void*) * entryCount;
    textures->textureName = (DiskTextures::TexName*)cursor;
    cursor += sizeof(DiskTextures::TexName) * entryCount;
    textures->textureInfo = (DiskTextures::TexInfo*)cursor;
    cursor += sizeof(DiskTextures::TexInfo) * entryCount;
    textures->totalPixelData = cursor;
    u32 pixelDataCursor = 0;



    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        PakTexture& tex = *(PakTexture*)(top + offset);

        textures->textureInfo[i].type = (DiskTextures::TexType)tex.typeId;
        textures->textureInfo[i].width = tex.width;
        textures->textureInfo[i].height = tex.height;
        memmove(&textures->textureName[i], tex.filename, sizeof(DiskTextures::TexName));

        u8* dataDest = textures->totalPixelData + pixelDataCursor;
        textures->textureData[i] = dataDest;

        if(tex.typeId == 6) {
            i32 texSize = tex.width * tex.height * 4;
            memmove(dataDest, top + offset + 80, texSize);
            pixelDataCursor += texSize;
        }
        else {
            i32 compressedSize = fb.fileSize - fileDesc[i].offset;
            if(i + 1 < entryCount) {
                compressedSize = fileDesc[i + 1].offset - fileDesc[i].offset;
            }
            i32 texSize = tex.width * tex.height * 2;
            i32 ret = zlib_decompress((const char*)top + offset + 80, compressedSize, dataDest, texSize);
            assert(ret == Z_OK);
            pixelDataCursor += texSize;
        }
    }

    LOG_SUCC("pak_texturesRead> all textures have been loaded to RAM");

    return true;
}

struct WorldBinEntry
{
    i32 varA;
    i32 varB;
    i32 varC;
};

bool bin_WorldRead(const char* filepath)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* cursor = (u8*)fb.block.ptr;
    u8* start = cursor;
    const u32 count = *(u32*)cursor;
    cursor += 4;

    for(i32 i = 0; i < count; ++i) {
        assert(cursor < (start + fb.fileSize));
        WorldBinEntry& entry = *(WorldBinEntry*)cursor;
        cursor += sizeof(WorldBinEntry);
        LOG_DBG("[%d] a=%d b=%d c=%d", i, entry.varA, entry.varB, entry.varC);
    }

    LOG_DBG("bin_WorldRead> done");

    return true;
}

struct FloorEntry
{
    i32 id;
    i32 pakTileIds;
    i32 varC;
    i32 nextTileId; // maybe?
};

bool pak_FloorRead(const char* filepath)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
    i32 entryCount = min(header->entryCount, 10000);
    //const i32 entryCount = header->entryCount;
    SubFileDesc* fileDesc = (SubFileDesc*)(top + sizeof(PakHeader));

    fileDesc++;
    entryCount--;

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        FloorEntry& fe = *(FloorEntry*)(top + offset);
        i32 pakTileId1 = fe.pakTileIds & 0x1FFFF;
        i32 pakTileId2 = fe.pakTileIds >> 17;

        LOG_DBG("[%d] %d %d %08x %d", i, pakTileId1, pakTileId2, fe.varC, fe.nextTileId);
    }


    LOG_DBG("pak_FloorRead> done");

    return true;
}

struct KeyxSub
{
    i32 int0;
    i32 fileOffset;
    i32 size;
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
    KeyxSub subs[32];
    u8 data[308];
};

static_assert(sizeof(KeyxSector) == 0x300, "sizeof(KeyxSector) := 0x300");

bool keyx_sectorsRead(const char* keyx_filepath, const char* wldx_filepath, DiskSectors* diskSectors)
{
    LOG_DBG("keyx_SectorsRead> start reading data...");

    // read keyx file
    FileBuffer fbKeyx = fileReadWhole(keyx_filepath);
    if(fbKeyx.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbKeyx.block.dealloc());

    // read wldx file
    FileBuffer fbWldx = fileReadWhole(wldx_filepath);
    if(fbWldx.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbWldx.block.dealloc());

    const u8* keyxData = (const u8*)fbKeyx.block.ptr;
    const u8* wldxData = (const u8*)fbWldx.block.ptr;

    PakHeader* header = (PakHeader*)keyxData;
    const i32 entryCount = header->entryCount;
    static u8 s_deflateOutput[Megabyte(2)];

    u64 deflatedDataTotalSize = 0;

    i32 keyxDataOffset = sizeof(PakHeader);
    for(i32 i = 0; i < entryCount; ++i) {
        KeyxSector& ks = *(KeyxSector*)(keyxData + keyxDataOffset);
        keyxDataOffset += sizeof(KeyxSector);
        const i32 uncompressedSize = ks.subs[15].size;
        deflatedDataTotalSize += uncompressedSize;
    }

    LOG_DBG("deflatedDataTotalSize=%llukb", deflatedDataTotalSize/1024);

    diskSectors->block = MEM_ALLOC(deflatedDataTotalSize);
    assert(diskSectors->block.ptr);
    diskSectors->entryData = (DiskSectors::WldxEntry*)diskSectors->block.ptr;
    u64 entryDataOffset = 0;

    keyxDataOffset = sizeof(PakHeader);
    for(i32 i = 0; i < entryCount; ++i) {
        KeyxSector& ks = *(KeyxSector*)(keyxData + keyxDataOffset);
        keyxDataOffset += sizeof(KeyxSector);

        /*LOG_DBG("[%d] %d px1=%d py1=%d, px2=%d py2=%d", i,
                ks.sectorId, ks.posX1, ks.posY1, ks.posX2, ks.posY2);
        LOG_DBG("sub_13={%d, %d, %d}", ks.subs[13].int0, ks.subs[13].fileOffset, ks.subs[13].size);
        LOG_DBG("uncompressedSize=%d deflatedDataCount=%d ucOff1=%d ucOff2=%d",
                ks.subs[15].size, ks.subs[2].size, ks.subs[11].fileOffset, ks.subs[12].fileOffset);*/

        const char* compData = (const char*)(wldxData + ks.subs[13].fileOffset);
        const i32 compSize = ks.subs[13].size;
        const i32 uncompressedSize = ks.subs[15].size;
        i32 ret = zlib_decompress(compData, compSize, s_deflateOutput, uncompressedSize);
        assert(ret == Z_OK);

        u8* entryDataNext = (u8*)diskSectors->entryData + entryDataOffset;

        DiskSectors::Sector sector;
        sector.posX1 = ks.posX1;
        sector.posX2 = ks.posX2;
        sector.posY1 = ks.posY1;
        sector.posY2 = ks.posY2;
        sector.wldxEntries = (DiskSectors::WldxEntry*)entryDataNext;
        sector.wldxEntryCount = uncompressedSize / sizeof(DiskSectors::WldxEntry);

        memmove((u8*)diskSectors->entryData + entryDataOffset, s_deflateOutput, uncompressedSize);
        entryDataOffset += uncompressedSize;

        diskSectors->sectors.pushPOD(&sector);
    }

    LOG_DBG("keyx_SectorsRead> done");

    return true;
}
