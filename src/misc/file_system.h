#pragma once

#include <cassert>

#include <string>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
#endif

#include "strutil.h"

namespace alioss {
    namespace file_system {
        const char* normalize_slash(char* path);
        std::string dirname(const std::string& path);
        void ls(const std::string& dir, std::vector<std::string>* files);

        // Returns true if path exists and is a folder
        bool is_folder(const std::string& path);

        // Gets the basename component of the file
        std::string basename(const std::string& file);
    }
}
