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
