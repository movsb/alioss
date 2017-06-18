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

        void ls(const std::string& dir, std::vector<std::string>* files)
        {
            if (!dir.size() || !files)
                return;

#ifdef _WIN32

            std::string dirname = file_system::dirname(dir);
            if (!dirname.empty()) {
                dirname += '/';
            }

            WIN32_FIND_DATA fd;
            HANDLE hfind;
            if ((hfind = ::FindFirstFile((dir).c_str(), &fd)) != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL
                        || fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY
                        )
                    {
                        std::string file = dirname + fd.cFileName;
                        // TODO warning on cast
                        normalize_slash(const_cast<char*>(file.c_str()));
#ifdef _WIN32
                        files->emplace_back(strutil::to_utf8(file));
#else
                        files->emplace_back(file);
#endif
                    }
                    else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        bool filtered =
                            fd.cFileName[0] == '.' && (
                            fd.cFileName[1] == '\0'
                            || fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'
                            )
                            ;
                        if (!filtered)
                            ls(dirname + fd.cFileName + '/', files);
                    }
                } while (::FindNextFile(hfind, &fd));
                ::FindClose(hfind);
            }
#else
            ::assert(0);
#endif
        }

        const char* normalize_slash(char* path)
        {
            auto s = path;
            while (*path) {
                if (*path == '\\')
                    *path = '/';

                ++path;
            }
            return s;
        }

        bool is_folder(const std::string& path)
        {
#ifdef _WIN32
            DWORD dwAttr = ::GetFileAttributes(path.c_str());
            return !!(dwAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
            assert(0);
#endif
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

    }
}
