#pragma once

#include <cassert>
#include <cstdio>

#define PANIC assert(0)

#define LOG_ANY(lvl, fmt, ...) dry::dbg::log(lvl, __FILE__, __LINE__, fmt, __VA_ARGS__)

#define LOG_INF(fmt, ...) LOG_ANY(dry::dbg::log_level::info,    fmt, __VA_ARGS__)
#define LOG_DBG(fmt, ...) LOG_ANY(dry::dbg::log_level::debug,   fmt, __VA_ARGS__)
#define LOG_WRN(fmt, ...) LOG_ANY(dry::dbg::log_level::warning, fmt, __VA_ARGS__)
#define LOG_ERR(fmt, ...) LOG_ANY(dry::dbg::log_level::error,   fmt, __VA_ARGS__)

#define PANIC_ASSERT(cond, fmt, ...) if (!(cond)) { LOG_ERR(fmt, __VA_ARGS__); PANIC; }
#define WRN_ASSERT(cond, fmt, ...) if (!(cond)) { LOG_WRN(fmt, __VA_ARGS__); }

namespace dry::dbg {

enum class log_level {
    info,
    debug,
    warning,
    error
};

template<typename... Args>
void log(log_level lvl, const char* file, uint32_t line, const char* format, Args&&... args) {
    const char* lvl_str = "\0";
    switch (lvl) {
        case log_level::info:    lvl_str = "\033[32m[INF "; break; // green
        case log_level::debug:   lvl_str = "\033[36m[DBG "; break; // cyan
        case log_level::warning: lvl_str = "\033[33m[WRN "; break; // yellow
        case log_level::error:   lvl_str = "\033[31m[ERR "; break; // red
        default:                 lvl_str = "\033[37m[ANY "; break; // white
    }

    const char* true_file = file + strlen(file);
    // just get the filename, __FILE__ is inconsistent
    while (true_file != file) {
        if (*true_file == '\\' || *true_file == '/') {
            ++true_file;
            break;
        }
        --true_file;
    }
    // can't have shit with iostream
    fprintf(stderr, "%s%s:%u]\033[0m", lvl_str, true_file, line);
    fprintf(stderr, format, args...);
}

}
