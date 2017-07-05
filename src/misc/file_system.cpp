#include <algorithm>

#include <shlobj.h>

#include "file_system.h"

namespace alioss {
    namespace file_system {

        std::string dirname(const std::string& path)
        {
            auto index = path.find_last_of("/\\");
            if (index != path.npos) {
                return path.substr(0, index - 0);
            }
            else {
                return "";
            }
        }

        void ls_files_inner(const std::wstring& _dir, std::vector<std::wstring>* files)
        {
            if (_dir.empty() || files == nullptr)
                return;

            auto dir= _dir;
            if(dir.back() != L'/') dir += L'/';

            WIN32_FIND_DATAW fd;
            HANDLE hfind;
            if ((hfind = ::FindFirstFileW((dir + L"*.*").c_str(), &fd)) != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY
                        )
                    {
                        files->emplace_back(dir + fd.cFileName);

                    }
                    else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        bool filtered =
                            fd.cFileName[0] == '.' && (
                            fd.cFileName[1] == '\0'
                            || fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'
                            )
                            ;
                        if(!filtered) {
                            ls_files_inner(dir + fd.cFileName, files);
                        }
                    }
                } while (::FindNextFileW(hfind, &fd));
                ::FindClose(hfind);
            }
        }

        void ls_files(const std::string& dir, std::vector<std::string>* files)
        {
            auto wdir = strutil::from_utf8(file_system::normalize_slash(dir));
            std::vector<std::wstring> wfiles;

            ls_files_inner(wdir, &wfiles);

            int prefix_len = dir.size() + (!dir.empty() && dir.back() == '/' ? 0 : 1);
            for(auto& file : wfiles) {
                auto f = strutil::to_utf8(file.c_str() + prefix_len);
                files->emplace_back(std::move(f));
            }
        }

        void normalize_slash(std::string* path)
        {
            std::replace(path->begin(), path->end(), '\\', '/');
        }


        std::string normalize_slash(const std::string& path)
        {
            auto dup = path;
            normalize_slash(&dup);
            return dup;
        }

        bool is_folder(const std::string& path)
        {
#ifdef _WIN32
            DWORD dwAttr = ::GetFileAttributes(path.c_str());
            return dwAttr != INVALID_FILE_ATTRIBUTES && !!(dwAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
            assert(0);
#endif
        }


        bool is_file(const std::string& path)
        {
            DWORD dwAttr = ::GetFileAttributes(path.c_str());
            return dwAttr != INVALID_FILE_ATTRIBUTES && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY);
        }

        std::string basename(const std::string& file)
        {
            auto off = file.rfind('/');
            if (off != file.npos) {
                return file.substr(off + 1);
            }
            else {
                return file;
            }
        }


        bool mkdir(const std::string& path)
        {
            auto wpath = strutil::from_utf8(path);
            if (!wpath.empty() && wpath.back() != '/'){
                wpath += '/';
            }
            size_t pos = 0, off = 0;
            while ((pos = wpath.find('/', off)) != wpath.npos) {
                if (pos > off) {
                    auto subdir = wpath.substr(0, pos);
                    if (subdir != L".") {
                        if (!::CreateDirectoryW(subdir.c_str(), nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS) {
                            return false;
                        }
                    }
                }
                off = pos + 1;
            }

            return true;
        }

    }
}
