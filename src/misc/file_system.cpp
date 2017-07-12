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

        void ls_files_impl(std::wstring& dir, std::vector<std::wstring>* files)
        {
            if (dir.empty() || files == nullptr)
                return;

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
                            ls_files_impl(dir + fd.cFileName, files);
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

            ls_files_impl(wdir, &wfiles);

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


        std::string ext_name(const std::string& file)
        {
            std::string ext;

            auto pd = file.rfind('.');

            // must have a period
            if(pd != file.npos) {
                // must not be the last char
                if(pd != file.size() - 1) {
                    // must not be the first char after an optional (back)slash char
                    auto ps = file.find_last_of("/\\");
                    if(ps == file.npos && pd > 0 || ps != file.npos && pd > ps + 1) {
                        ext = file.substr(pd);
                    }
                }
            }

            return ext;
        }

        std::string mime(const std::string& ext, const std::string& def /*= "octet/stream"*/)
        {
            struct Index {
                enum Value {
                    Binary,

                    Plain,

                    XML,
                    HTML,
                    CSS,
                    Javascript,
                    JSON,

                    GIF,
                    JPEG,
                    PNG,

                    PDF,
                    DOC,
                    XLS,
                    PPT,

                    _END,
                };
            };

            typedef Index _;

            static const char* _known_mimes[Index::_END];
            {
                static bool _inited = false;
                if(!_inited) {
                    _inited = true;

                    auto& m = _known_mimes;

                    m[_::Binary]        = "octet/stream"                    ;

                    m[_::Plain]         = "text/plain"                      ;

                    m[_::XML]           = "text/xml"                        ;
                    m[_::HTML]          = "text/html"                       ;
                    m[_::CSS]           = "text/css"                        ;
                    m[_::Javascript]    = "text/javascript"                 ;
                    m[_::JSON]          = "text/json"                       ;

                    m[_::GIF]           = "image/gif"                       ;
                    m[_::JPEG]          = "image/jpeg"                      ;
                    m[_::PNG]           = "image/png"                       ;

                    m[_::PDF]           = "application/pdf"                 ;
                    m[_::DOC]           = "application/msword"              ;
                    m[_::XLS]           = "application/vnd.ms-excel"        ;
                    m[_::PPT]           = "application/vnd.ms-powerpoint"   ;
                }

            }

            static const std::map<std::string, int> _known_types = {
                {".txt",   _::Plain},

                {".xml",    _::XML},
                {".html",   _::HTML},
                {".css",    _::CSS},
                {".js",     _::Javascript},
                {".json",   _::JSON},

                {".gif",    _::GIF},
                {".jpg",    _::JPEG},
                {".jpeg",   _::JPEG},
                {".png",    _::PNG},

                {".pdf",    _::PDF},
                {".doc",    _::DOC},
                {".docx",   _::DOC},
                {".xls",    _::XLS},
                {".xlss",   _::XLS},
                {".ppt",    _::PPT},
                {".pptx",   _::PPT},
            };

            auto it = _known_types.find(strutil::to_lower(ext));
            if(it == _known_types.cend()) {
                return def;
            }

            return _known_mimes[it->second];
        }

    }
}
