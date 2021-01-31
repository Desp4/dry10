#pragma once

#include <cassert>
#include <cstdio>

enum LogLevel
{
    Log_Info,
    Log_Debug,
    Log_Warning,
    Log_Error
};

#define PANIC assert(0)

#define LOG_ANY(lvl, fmt, ...) logFormatted(lvl, __FILE__, __LINE__, fmt, __VA_ARGS__)

#define LOG_INF(fmt, ...) LOG_ANY(Log_Info, fmt, __VA_ARGS__)
#define LOG_DBG(fmt, ...) LOG_ANY(Log_Debug, fmt, __VA_ARGS__)
#define LOG_WRN(fmt, ...) LOG_ANY(Log_Warning, fmt, __VA_ARGS__)
#define LOG_ERR(fmt, ...) LOG_ANY(Log_Error, fmt, __VA_ARGS__)

#define PANIC_ASSERT(cond, fmt, ...) if (!(cond)) { LOG_ERR(fmt, __VA_ARGS__); PANIC; }

void log(LogLevel lvl, const char* file, int line, const char* msg);

template<typename... Args>
void logFormatted(LogLevel lvl, const char* file, int line, const char* format, Args&&... args)
{
    static char buffer[1024];
    std::snprintf(buffer, sizeof buffer, format, args...);
    log(lvl, file, line, buffer);
}