#pragma once

#include <cassert>

#include <string>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
#endif

#include "strutil.h"

namespace alioss {
    namespace os {
        void ls_files(const std::string& dir, std::vector<std::string>* files) {
            if(!dir.size() || !files)
                return;

#ifdef _WIN32

            WIN32_FIND_DATA fd;
            HANDLE hfind;
            if((hfind = ::FindFirstFile((dir + "*").c_str(), &fd)) != INVALID_HANDLE_VALUE) {
                do {
                    if(fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY
                        )
                    {
                        std::string file = dir + fd.cFileName;
                        strutil::normalize_slash(const_cast<char*>(file.c_str()));
                        files->push_back(file);
                    }
                    else if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        bool filtered =
                            fd.cFileName[0] == '.' && (
                                fd.cFileName[1] == '\0'
                                || fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'
                                )
                            ;
                        if(!filtered)
                            ls_files(dir + fd.cFileName + '/', files);
                    }
                } while(::FindNextFile(hfind, &fd));
                ::FindClose(hfind);
            }
#else
            ::assert(0);
#endif
        }
    }
}
