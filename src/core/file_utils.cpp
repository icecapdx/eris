#include "file_utils.h"
#include <fstream>
#include <sstream>
#include <cstring>

#ifndef VITA_BUILD
#  include <SDL.h>
#endif

std::string resolve_path(const std::string& path) {
    if (!path.empty() && (path[0] == '/' || path.find(':') != std::string::npos))
        return path;

#ifdef VITA_BUILD
    return std::string(ERIS_DATA_PATH) + path;
#else
    static std::string s_base;
    if (s_base.empty()) {
        char* bp = SDL_GetBasePath();
        if (bp) { s_base = bp; SDL_free(bp); }
        else    { s_base = "./"; }
    }
    return s_base + path;
#endif
}

std::string read_file(const std::string& path) {
    std::string full = resolve_path(path);
    std::ifstream f(full, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
