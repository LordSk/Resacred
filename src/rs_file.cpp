#include "rs_file.h"
#include "rs_window.h"
#include "rs_array.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include "zlib.h"
#include "rs_jobs.h"

enum class FileReqType: i32 {
    INVALID = 0,
    READ_ABSOLUTE
};

struct AsyncFileRequestDesc
{
    FileReqType type = FileReqType::INVALID;
    DiskFile* file = nullptr;
    u8* buff = nullptr;
    i64 size = 0;
    u8* out = nullptr;
    AtomicCounter* counter = nullptr;
};

typedef Array<AsyncFileRequestDesc,127> AsyncFileRequestDescArray;
struct AsyncFileQueue
{
    AsyncFileRequestDescArray queueBuffers[2];
    AsyncFileRequestDescArray* queue = &queueBuffers[0];
    Mutex acquireMutex;

    // swap front and back queue
    bool swapQueues()
    {
        if(queue->count() == 0) {
            threadSleep(15);
            return false;
        }

        acquireMutex.lock();

        if(queue == &queueBuffers[0]) {
            queue = &queueBuffers[1];
        }
        else {
            queue = &queueBuffers[0];
        }

        acquireMutex.unlock();
        return true;
    }

    // should be the only function acting on the back queue
    void handleAllBackRequests()
    {
        AsyncFileRequestDescArray* backQueue = 0;

        if(queue == &queueBuffers[0]) {
            backQueue = &queueBuffers[1];
        }
        else {
            backQueue = &queueBuffers[0];
        }

        const i32 backQueueCount = backQueue->count();

        //LOG_DBG("FileIO> Handling queue %#d, count=%d", backQueue - &queueBuffers[0], backQueueCount);

        for(i32 i = 0; i < backQueueCount; ++i) {
            AsyncFileRequestDesc& desc = (*backQueue)[i];

            switch(desc.type) {
                case FileReqType::READ_ABSOLUTE: {
                    assert(desc.out);
                    fileReadFromPos(desc.file, (i64)desc.buff, desc.size, desc.out);

                    if(desc.counter) {
                        desc.counter->decrement();
                    }
                    break;
                }
                default:
                    assert(0);
                    break;
            }
        }

        backQueue->clearPOD();
    }

    AsyncFileRequest newRequest(FileReqType type, DiskFile* file, u8* buff, i64 size, u8* out,
                                AtomicCounter* counter) {
        // fileIO is getting clogged, block until unclogged
        while(queue->count() > 127) {
            //_mm_pause(); // Actually very important?
            threadSleep(15);
        }

        acquireMutex.lock();

        assert(file->handle);
        assert(buff);
        assert(size > 0);

        i32 ruid = queue->count();
        AsyncFileRequest req;
        req.requestUID = ruid;

        AsyncFileRequestDesc desc;
        desc.type = type;
        desc.file = file;
        desc.buff = buff;
        desc.out = out;
        desc.size = size;
        desc.counter = counter;
        queue->pushPOD(&desc);

        acquireMutex.unlock();
        return req;
    }
};

static AsyncFileQueue* AFQ;

unsigned long thread_fileIO(void*)
{
    LOG("thread_fileIO started [%x]", threadGetId());

    AsyncFileQueue asyncFileQueue;
    AFQ = &asyncFileQueue;

    LOG_SUCC("FileIO> initialized");

    Window& client = *get_clientWindow();
    while(client.isRunning) {
        if(AFQ->swapQueues()) {
            AFQ->handleAllBackRequests();
        }
        else {
            threadSleep(1);
            /*SwitchToThread();
            YieldProcessor();*/
        }
    }

    return 0;
}


bool fileOpenToRead(const char* path, DiskFile* file)
{
#ifdef CONF_WINDOWS
    HANDLE hFile = CreateFileA(path,
							   GENERIC_READ,
							   0 /*FILE_SHARE_READ*/, // TODO: change this to 0
                               NULL,
                               OPEN_EXISTING,
							   FILE_ATTRIBUTE_READONLY|FILE_FLAG_OVERLAPPED,
                               NULL);

    if(hFile != INVALID_HANDLE_VALUE) {
        i32 len = strlen(path);
        memmove(file->path, path, len);
        file->path[len] = 0;
        file->handle = hFile;

        // get size
        LARGE_INTEGER size;
        GetFileSizeEx(hFile, &size);
        file->size = size.QuadPart;
        LOG_SUCC("file '%s' (%lldko) open for reading", path, file->size/1024);
        return true;
    }
    LOG_ERR("ERROR: opening file '%s' (%d)", path, GetLastError());
    return false;
#endif
}

void fileClose(DiskFile* file)
{
#ifdef CONF_WINDOWS
	if(file->handle) {
		CloseHandle(file->handle);
		file->handle = 0;
	}
#endif
}

void fileReadFromPos(const DiskFile* file, i64 from, i64 size, void* dest)
{
#ifdef CONF_WINDOWS
    assert(file->handle);
    assert(dest);

    // read
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Pointer = (void*)from;
	overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    DWORD unused;
	BOOL r = ReadFile(file->handle, dest, size, NULL, &overlapped);
	if(!r && GetLastError() != ERROR_IO_PENDING) {
		LOG_ERR("ERROR: reading file '%s' (%d)", file->path, GetLastError());
		assert_msg(0, "Error reading file");
	}
	GetOverlappedResult(file->handle, &overlapped, &unused, TRUE);
#endif
}

void fileReadAdvance(DiskFile* file, i64 size, void* dest)
{
#ifdef CONF_WINDOWS
    assert(file->handle);
    assert(dest);

	DWORD unused;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = file->cursor;
	overlapped.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

	BOOL r = ReadFile(file->handle, dest, size, NULL, &overlapped); // advances file pointer
	if(!r && GetLastError() != ERROR_IO_PENDING) {
		LOG_ERR("ERROR: reading file '%s' (%d)", file->path, GetLastError());
		assert_msg(0, "Error reading file");
	}
	GetOverlappedResult(file->handle, &overlapped, &unused, TRUE);

    file->cursor += size;
#endif
}

AsyncFileRequest fileAsyncReadAbsolute(const DiskFile* file, i64 start, i64 size, u8* out,
                                       AtomicCounter* counter)
{
    assert(((i64)start + size) < file->size);
    assert(out);
#if 0
    return AFQ->newRequest(FileReqType::READ_ABSOLUTE, (DiskFile*)file, (u8*)start, size, out, counter);
#else
	struct JobData {
		const DiskFile* file;
		i64 start;
		i64 size;
		u8* out;
		AtomicCounter* pCounter;
	};

	JobData jd = JobData{file, start, size, out, counter};

	jobRunEx(&jd, sizeof(JobData), [](void* pData) {
		JobData data = *(JobData*)pData;

		fileReadFromPos(data.file, data.start, data.size, data.out);

		data.pCounter->decrement();
	}, counter);

	return {};
#endif
}




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

    fb.size = len;
    fb.block = pAlloc->ALLOC(len + 1);
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
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

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
            if(copySize <= 0) {
                break;
            }
        }

        strm.avail_in = copySize;
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

#if 0
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
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

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

        textures->textureInfo[i].type = (PakTextureType)tex.typeId;
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
            i32 compressedSize = fb.size - fileDesc[i].offset;
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
#endif

bool pak_textureRead(char* fileBuff, i64 size, i32* out_width, i32* out_height, i32* out_type,
                     u8* out_data, i32* out_size, char* out_name)
{
    PakTextureHeader& tex = *(PakTextureHeader*)fileBuff;

    *out_type = tex.typeId;
    *out_width = tex.width;
    *out_height = tex.height;
#ifdef CONF_DEBUG
    memmove(out_name, tex.filename, 32);
#endif

    if(tex.typeId == 6) {
        i32 texSize = tex.width * tex.height * 4;
        memmove(out_data, fileBuff + 80, texSize);
        *out_size = texSize;
    }
    else {
        i32 compressedSize = size;
        i32 texSize = tex.width * tex.height * 2;
        *out_size = texSize;
        i32 ret = zlib_decompress(fileBuff + 80, compressedSize, out_data, texSize);
        assert(ret == Z_OK);
    }

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
        assert(cursor < (start + fb.size));
        WorldBinEntry& entry = *(WorldBinEntry*)cursor;
        cursor += sizeof(WorldBinEntry);
        LOG_DBG("[%d] a=%d b=%d c=%d", i, entry.varA, entry.varB, entry.varC);
    }

    LOG_DBG("bin_WorldRead> done");

    return true;
}

bool pak_FloorRead(const char* filepath)
{
    FileBuffer fb = fileReadWhole(filepath);
    if(fb.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fb.block.dealloc());

    u8* top = (u8*)fb.block.ptr;
    PakHeader* header = (PakHeader*)top;
	i32 entryCount = MIN(header->entryCount, 10000);
    //const i32 entryCount = header->entryCount;
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

    fileDesc++;
    entryCount--;

    for(i32 i = 0; i < entryCount; ++i) {
        i32 offset = fileDesc[i].offset;
        FloorEntry& fe = *(FloorEntry*)(top + offset);
        i32 pakTileId1 = fe.pakTileIds & 0x1FFFF;
        i32 pakTileId2 = fe.pakTileIds >> 17;

        LOG_DBG("[%d] %d %d %08x %d", i, pakTileId1, pakTileId2, fe.varC, fe.nextFloorId);
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
        assert(uncompressedSize < Megabyte(2));
    }

    LOG_DBG("deflatedDataTotalSize=%llukb entryCount=%d", deflatedDataTotalSize/1024, entryCount);

    diskSectors->block = MEM_ALLOC(deflatedDataTotalSize);
    assert(diskSectors->block.ptr);
    diskSectors->entryData = (WldxEntry*)diskSectors->block.ptr;
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
        sector.id = ks.sectorId;
        sector.posX1 = ks.posX1;
        sector.posX2 = ks.posX2;
        sector.posY1 = ks.posY1;
        sector.posY2 = ks.posY2;
        sector.wldxEntries = (WldxEntry*)entryDataNext;
        sector.wldxEntryCount = uncompressedSize / sizeof(WldxEntry);

        memmove(entryDataNext, s_deflateOutput, uncompressedSize);
        entryDataOffset += uncompressedSize;

        diskSectors->sectors.pushPOD(&sector);
    }

    LOG_DBG("keyx_SectorsRead> done");
    return true;
}

void deflateSectorData(DiskFile* file, i32 offset, i32 compSize, i32 uncompSize, u8* outFileBuffer, u8* outData)
{
    fileReadFromPos(file, offset, compSize, outFileBuffer);
    i32 ret = zlib_decompress((char*)outFileBuffer, compSize, (u8*)outData, uncompSize);
    assert(ret == 0); // Z_OK
}

bool pak_staticRead(const char* path, Array<PakStatic>* out)
{
    LOG_DBG("pak_staticRead> start reading data...");

    // read keyx file
    FileBuffer fbStatic = fileReadWhole(path);
    if(fbStatic.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbStatic.block.dealloc());

    const u8* top = (const u8*)fbStatic.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

    LOG_DBG("pak_staticRead> entryCount=%d", entryCount);

    out->reserve(entryCount);
    out->allocator = nullptr; // TODO: remove this
    out->pushPOD((PakStatic*)(top + fileDesc[0].offset), entryCount);

    return true;
}

bool pak_mixedRead(PakMixedFileData* out)
{
	LOG_DBG("pak_MixedRead> start reading data...");

    // read keyx file
    FileBuffer fbMixed = fileReadWhole("../sacred_data/mixed.pak");
    if(fbMixed.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbMixed.block.dealloc());

    const u8* top = (const u8*)fbMixed.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

    LOG_DBG("pak_MixedRead> entryCount=%d", entryCount);

    out->descCount = entryCount;
    i64 blockSize = entryCount * (sizeof(PakMixedDesc));

    for(i32 i = 0; i < entryCount; ++i) {
       if(fileDesc[i].size != 16) {
            i32 mixedArrayCount = (fileDesc[i].size - sizeof(PakMixedDesc)) / sizeof(PakMixedEntry);
            blockSize += mixedArrayCount * sizeof(PakMixedData);
        }
    }

    out->block = MEM_ALLOC(blockSize);
    assert(out->block.ptr);
    out->desc = (PakMixedDesc*)out->block.ptr;
    out->mixed = (PakMixedData*)(out->desc + entryCount);

    LOG_DBG("pak_MixedRead> blockSize=%lld", blockSize);

    for(i32 i = 0; i < entryCount; ++i) {
        const i32 offset = fileDesc[i].offset;

        if(fileDesc[i].size == 16) {
            out->desc[i] = {};
        }
        else {
            PakMixedDesc& desc = *(PakMixedDesc*)(top + offset);
            out->desc[i] = desc;
            out->desc[i].mixedDataId = out->mixedCount;

            for(i32 m = 0; m < desc.count; ++m) {
                out->mixed[out->mixedCount++] = ((PakMixedEntry*)(&desc + 1))[m].data;
                PakMixedData& e = out->mixed[out->mixedCount-1];
                assert(e.zero[0] == 0);
                assert(e.zero[1] == 0);
            }
        }
    }

    LOG_SUCC("pak_MixedRead> finished reading (desc=%d mixed=%d)", out->descCount, out->mixedCount);
    return true;
}

bool pak_itemRead(const char* path, Array<PakItemType>* out)
{
    FileBuffer fbItem = fileReadWhole(path);
    if(fbItem.error != FileError::NO_FILE_ERROR) {
        return false;
    }
    defer(fbItem.block.dealloc());

    const u8* top = (const u8*)fbItem.block.ptr;
    PakHeader* header = (PakHeader*)top;
    const i32 entryCount = header->entryCount;
    PakSubFileDesc* fileDesc = (PakSubFileDesc*)(top + sizeof(PakHeader));

    LOG_DBG("pak_itemRead> entryCount=%d", entryCount);

    out->reserve(entryCount);
    out->allocator = nullptr; // TODO: remove this
    out->pushPOD((PakItemType*)(top + fileDesc[0].offset), entryCount);

    /*for(i32 i = 0; i < out->count(); i++) {
        assert(i == out->data()[i].mixedId);
    }*/

    return true;
}
