#pragma once
#include "rs_base.h"
#include "rs_thread.h"
#include <stdio.h>

#define UNIX_CONSOLE_COLOR_RED     "\x1b[31m"
#define UNIX_CONSOLE_COLOR_GREEN   "\x1b[32m"
#define UNIX_CONSOLE_COLOR_YELLOW  "\x1b[33m"
#define UNIX_CONSOLE_COLOR_BLUE    "\x1b[34m"
#define UNIX_CONSOLE_COLOR_MAGENTA "\x1b[35m"
#define UNIX_CONSOLE_COLOR_CYAN    "\x1b[36m"
#define UNIX_CONSOLE_COLOR_RESET   "\x1b[0m"

enum: u32 {
    LOG_COLOR_TEXT=0,
    LOG_COLOR_ERROR,
    LOG_COLOR_SUCCESS,
    LOG_COLOR_WARN,
    LOG_COLOR_BLUE,
    LOG_COLOR_MAGENTA,
    LOG_COLOR_CYAN,
};

struct Logger
{
    enum: u32 {
        STDIN=0,
        STDOUT=1,
        STDERR=2,

        MAX_LINE_SIZE=4096,
    };

    char _formatStr[MAX_LINE_SIZE];
    i32 _formattedLen = 0;
    FILE* _pFile = nullptr;
    MutexSpin _mutex;

    Logger(const char* logName = "log");
    ~Logger();

    void logf(u32 typeColor, const char* filename, i32 line, const char* format, ...);
};

inline Logger& getGlobalLogger()
{
    static Logger logger;
    return logger;
}

#define LOGGER_INIT() getGlobalLogger()

#define LOG(format, ...) getGlobalLogger().logf(LOG_COLOR_TEXT,\
    __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) getGlobalLogger().logf(LOG_COLOR_WARN,\
    __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_ERR(format, ...) getGlobalLogger().logf(LOG_COLOR_ERROR,\
    __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_SUCC(format, ...) getGlobalLogger().logf(LOG_COLOR_SUCCESS,\
    __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_THREAD(format, ...) getGlobalLogger().logf(LOG_COLOR_CYAN,\
	__FILE__, __LINE__, format, ##__VA_ARGS__)

#ifndef NDEBUG
    #define LOG_DBG(format, ...) getGlobalLogger().logf(LOG_COLOR_WARN,\
        __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
    #define LOG_DBG(format, ...)
#endif
