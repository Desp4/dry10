#include "fs.hpp"

#include "dbg/log.hpp"

#ifdef WIN32
  #include <Windows.h>
#endif

namespace dry {

#ifdef WIN32
std::filesystem::path init_exe_dir() {
    CHAR path[MAX_PATH]; // TODO : NOTE : no unicode
    const auto ret = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (ret == 0) {
        LOG_ERR("Could not resolve current executable path");
        dbg::panic();
    }
    return std::filesystem::path{ path }.parent_path();
}


#endif

const std::filesystem::path g_exe_dir = init_exe_dir();

}