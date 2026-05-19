#pragma once
#include <string>

#ifdef VITA_BUILD
#  define ERIS_DATA_PATH  "ux0:data/eris/"
#else
#  define ERIS_DATA_PATH  ""
#endif

std::string read_file(const std::string& path);
std::string resolve_path(const std::string& path);
