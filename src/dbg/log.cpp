#include "log.hpp"

#include <iostream>

const char* logHeader(LogLevel lvl)
{
    switch (lvl)
    {
    case Log_Info:    return "\033[32m[INF "; // green
    case Log_Debug:   return "\033[36m[DBG "; // cyan
    case Log_Warning: return "\033[33m[WRN "; // yellow
    case Log_Error:   return "\033[31m[ERR "; // red
    default:          return "\033[37m[ANY "; // white
    }
}

void log(LogLevel lvl, const char* file, int line, const char* msg)
{

    const char* filename = file + std::strlen(file);
    while (filename != file)
    {
        if (*filename == '\\' || *filename == '/')
        {
            ++filename;
            break;
        }
        --filename;
    }

    std::cerr << logHeader(lvl) << filename << ':' << line << "]\033[0m ";
    std::cerr << msg << std::endl;
}