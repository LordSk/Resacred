#include "rs_logger.h"
#include "rs_string.h"
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>
#include <stdarg.h>

Logger::Logger(const char* logName)
{
#ifdef _WIN32
    _mkdir("./logs");
#else
    mkdir("./logs", 2222);
#endif

    String<256> logFileName;
    logFileName.setf("./logs/%s", logName);
    time_t t = time(0);
    struct tm* tt = localtime(&t);
    logFileName.appendf("_%d-%d-%d_%d-%d-%d.txt",
                        tt->tm_mday, tt->tm_mon+1, tt->tm_year+1900,
                        tt->tm_hour, tt->tm_min, tt->tm_sec);
    _pFile = fopen(logFileName.c_str(), "w");
    assert(_pFile);
}

Logger::~Logger()
{
    assert_msg(_pFile, "Logger was not initialized, yet used");
    fclose(_pFile);
    _pFile = 0;
}

//__attribute__((format(printf, 3, 4)))
void Logger::logf(u32 typeColor, const char* filename, i32 lineNumber, const char* format, ...)
{
    _mutex.lock();

    String<MAX_LINE_SIZE> logLineStr;
    logLineStr.allocator = nullptr;

    i32 colorOffset = 5;
    switch(typeColor) {
        case LOG_COLOR_TEXT:      logLineStr.append(UNIX_CONSOLE_COLOR_RESET, 4); colorOffset = 4; break;
        case LOG_COLOR_ERROR:     logLineStr.append(UNIX_CONSOLE_COLOR_RED, 5); break;
        case LOG_COLOR_SUCCESS:   logLineStr.append(UNIX_CONSOLE_COLOR_GREEN, 5); break;
        case LOG_COLOR_WARN:      logLineStr.append(UNIX_CONSOLE_COLOR_YELLOW, 5); break;
        case LOG_COLOR_BLUE:      logLineStr.append(UNIX_CONSOLE_COLOR_BLUE, 5); break;
        case LOG_COLOR_MAGENTA:   logLineStr.append(UNIX_CONSOLE_COLOR_MAGENTA, 5); break;
        case LOG_COLOR_CYAN:      logLineStr.append(UNIX_CONSOLE_COLOR_CYAN, 5); break;
    }

    i32 filenameLen = strLen(filename);
    i32 onlyFileStart = 0;
    for(i32 i = filenameLen-1; i >= 0; i--) {
        if(filename[i] == '/' || filename[i] == '\\') {
            onlyFileStart = i+1;
            break;
        }
    }
    filenameLen -= onlyFileStart;

    logLineStr.append("[", 1);

    logLineStr.appendf("%20s", filename + onlyFileStart);
    logLineStr.appendf(":%5d", lineNumber);

    logLineStr.append("] ", 2);

    va_list args;
    va_start(args, format);
    _formattedLen = vsnprintf(_formatStr, MAX_LINE_SIZE - logLineStr.len() - 1, format, args);
    va_end(args);

    logLineStr.append(_formatStr, _formattedLen);
    logLineStr.append("\n", 1);

    _write(STDOUT, logLineStr.c_str(), logLineStr.len());

#ifndef LOGGER_DONT_WRITE
    assert_msg(_pFile, "Logger was not initialized, yet used");
    fwrite(logLineStr.c_str() + colorOffset, sizeof(char), logLineStr.len() - colorOffset, _pFile);
    fflush(_pFile); // TODO: do this in another thread?
#endif

    _mutex.unlock();
}
