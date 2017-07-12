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
        // Replaces all occurrences of '\' to '/'
        void normalize_slash(std::string* path);
        std::string normalize_slash(const std::string& path);

        // Returns the name component of a path
        std::string dirname(const std::string& path);

        // Lists files in directory
        void ls_files(const std::string& dir, std::vector<std::string>* files);

        // Returns true if path exists and is a folder
        bool is_folder(const std::string& path);

        // Returns true if path exists and is a file
        bool is_file(const std::string& path);

        // Gets the basename component of the file
        std::string basename(const std::string& file);

        // Creates directories recursively
        bool mkdir(const std::string& path);

        // Gets the extension name of a file
        std::string ext_name(const std::string& file);

        // Gets the MIME type of an extension (exts are NOT case sensitive)
        std::string mime(const std::string& ext, const std::string& def = "octet/stream");
    }
}
