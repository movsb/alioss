#include <cstdio>
#include <csignal>
#include <ctime>
#include <cstdlib>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <memory>

#ifdef _WIN32
#include <direct.h>
#endif

#include "socket.h"
#include "osserror.h"
#include "service.h"
#include "bucket.h"
#include "ossmeta.h"
#include "misc/strutil.h"
#include "misc/stream.h"
#include "misc/file_system.h"
#include "misc/time.h"
#include "misc/argcv.hpp"

using namespace alioss;

#define  OSS_BUCKET_NAME_ENV        "__ALIOSS_BUCKET"       // twofei-test
#define  OSS_BUCKET_LOCATION_ENV    "__ALIOSS_LOCATION"     // oss-cn-shenzhen

static std::string g_oss_bucket_name;
static std::string g_oss_bucket_location;
static accesskey g_key;

static bool read_access_key(accesskey* key)
{
	std::string file;
#ifdef _WIN32
	char _path[260] = { 0 };
	GetModuleFileName(NULL, _path, sizeof(_path));
	strcpy(strrchr(_path, '\\') + 1, "alioss.key");
	file = _path;
#else
	file = getenv("HOME");
	file += "/.alioss.key";
#endif

	std::ifstream ifs(file);
	std::string keyid, keysec;
	if (ifs){
		char buf1[128],buf2[128];
		ifs.getline(buf1, sizeof(buf1));
		ifs.getline(buf2, sizeof(buf2));

		keyid = buf1;
		keysec = buf2;
	}
	else{
		std::cout << "Input access key ID: ";
		std::getline(std::cin, keyid, '\n');
		std::cout << "Input access key secret: ";
		std::getline(std::cin, keysec, '\n');
	}

	if(keyid.size() != 16 || keysec.size() != 30) {
		throw std::string("Your alioss key/sec is invalid!");
	}

	key->set_key(keyid.c_str(), keysec.c_str());
	return true;
}


static bool find_file(const std::vector<meta::content>& files, const std::string& needle)
{
    return std::find_if(files.cbegin(), files.cend(), [&needle](const meta::content& file) {
        return file.key() == needle;
    }) != files.cend();
}

static bool find_folder(const std::vector<std::string>& folders, const std::string& needle)
{
    return std::find_if(folders.cbegin(), folders.cend(), [&needle](const std::string& folder) {
        return folder == needle;
    }) != folders.cend();
}

static void help()
{
    std::cerr << "alioss - a simple Ali OSS manager\n"
        << "\n"
        << "Syntax:\n\n"
        << "    <type>   <command>    <[parameters...]>\n\n"
        << "    bucket   list\n"
        << "\n"
        << "    object   list         <directory>\n"
        << "    object   head         <file>\n"
        << "    object   sign         <file>              [expiration]\n"
        << "\n"
        << "    object   download     <file>              [file/directory]\n"
        << "    object   download     <directory>         [directory]\n"
        << "\n"
        << "    object   upload       <file>              <file>\n"
        << "    object   upload       <directory>         <file>\n"
        << "    object   upload       <directory>         <directory>\n"
        << "\n"
        << "    object   delete       <file/directory>\n"
        << "\n"
        ;
}

static int eval(int argc, const char*const * argv)
{
    if (argc < 1) {
        return 1;
    }

    try {
        auto object = std::string(argv[0]);
        if (object == "bucket") {
            service::service svc(g_key);

            svc.set_endpoint(meta::oss_root_server);

            if (argc >= 2) {
                auto command = std::string(argv[1]);
                if (command == "list") {
                    std::vector<meta::bucket> buckets;
                    svc.list_buckets(&buckets);
                    for (const auto& bkt : buckets) {
                        std::cout
                            << "Name          : " << bkt.name() << std::endl
                            << "Location      : " << bkt.location() << std::endl
                            << "Creation Date : " << bkt.creation_date() << std::endl
                            << "End Point     : " << meta::make_endpoint(bkt.location()) << std::endl
                            << "Public Host   : " << meta::make_public_host(bkt.name(), bkt.location()) << std::endl
                            << std::endl
                            ;
                    }
                }
            }
        }
        else if (object == "object") {
            bucket::bucket bkt(g_key);

            bkt.set_endpoint(g_oss_bucket_name, g_oss_bucket_location);

            if (argc >= 2) {
                auto command = std::string(argv[1]);
                if (command == "list") {
                    if (argc >= 3) {
                        auto folder = std::string(argv[2]);
                        std::vector<meta::content> files;
                        std::vector<std::string> folders;

                        bkt.list_objects(folder, true, &files, &folders);

                        std::cout << "Folders:\n";
                        for (const auto& f : folders) {
                            std::cout << "  " << f << std::endl;
                        }

                        std::cout << "\n";

                        std::cout << "Files:\n";
                        for (const auto& f : files) {
                            std::cout << "  " << f.key() << std::endl
                                << "    Last modified : " << f.last_modified() << '\n'
                                << "    Type          : " << f.type() << '\n'
                                << "    Size          : " << f.size() << '\n'
                                << "    ETag          : " << f.e_tag() << '\n'
                                << "    Storage class : " << f.storage_class() << '\n'
                                << "\n"
                                ;
                        }
                    }
                }
                else if (command == "head") {
                    if (argc >= 3) {
                        auto file = argv[2];
                        auto head = bkt.head_object(file);
                        head.dump([](size_t i, const std::string& key, const std::string& val){
                            std::printf("%-25s: %s\n", key.c_str(), val.c_str());
                            return true;
                        });
                    }
                }
                else if (command == "sign") {
                    if (argc >= 3) {
                        auto file = argv[2];

                        std::string url;
                        url += "http://";
                        url += meta::make_public_host(g_oss_bucket_name, g_oss_bucket_location);

                        if (argc >= 4) {
                            auto parse_expiration = [](const std::string& expr) {
                                int day = 0, hour = 0, minute = 0, second = 0;

                                for (size_t i = 0; i < expr.size();) {
                                    int num = 0;
                                    char unit = 's';

                                    for (; i < expr.size(); i++) {
                                        if ('0' <= expr[i] && expr[i] <= '9') {
                                            num *= 10;
                                            num += expr[i] - '0';
                                        }
                                        else {
                                            break;
                                        }
                                    }

                                    if (i < expr.size()) {
                                        unit = expr[i];
                                        i++;
                                    }

                                    int* p;

                                    switch (unit) {
                                    case 'd': p = &day; break;
                                    case 'h': p = &hour; break;
                                    case 'm': p = &minute; break;
                                    case 's': p = &second; break;
                                    default: return -1;
                                    }

                                    *p = num;
                                }

                                int expiration
                                    = day * (24 * 60 * 60)
                                    + hour * (60 * 60)
                                    + minute * (60)
                                    + second * (1)
                                    ;

                                return expiration;
                            };

                            auto expr = parse_expiration(argv[3]);
                            if (expr == -1) {
                                std::cerr << "Bad expiration." << std::endl;
                                return -1;
                            }

                            auto current = std::time(nullptr);
                            auto expiration = current + expr;
                            auto expr_str = std::to_string(expiration);

                            std::map<std::string, std::string> query = {
                                { "OSSAccessKeyId", g_key.key() },
                                { "Expires", expr_str },
                                { "Signature", sign_url(g_key, expiration, std::string("/") + g_oss_bucket_name + file) },
                            };

                            url += strutil::make_uri(file, query);
                        }
                        else {
                            url += strutil::make_uri(file, {});
                        }

                        std::cout << url << std::endl;
                    }
                }
                else if (command == "download") {
                    if (argc >= 3) {
                        auto input_path = file_system::normalize_slash(argv[2]);
                        auto input_dir = file_system::dirname(input_path);

                        if (input_path[0] != '/') {
                            throw "Invalid path.";
                        }

                        std::vector<meta::content> files;
                        std::vector<std::string> folders;

                        bkt.list_objects(input_dir, false, &files, &folders);

                        bool is_download_file = input_path.back() != '/';

                        if (is_download_file && !find_file(files, input_path)) {
                            is_download_file = false;
                        }

                        if (!is_download_file) {
                            auto input_path2 = input_path.back() == '/' ? input_path : input_path + '/';
                            auto has_dir = input_path2 == "/" || find_folder(folders, input_path2);
                            if (!has_dir) {
                                std::cerr << "No such file or folder: " << input_path << std::endl;
                                return 1;
                            }
                            else {
                                input_dir = input_path2;
                            }
                        }

                        if (is_download_file) {
                            auto local_dir = std::string(".");
                            auto local_name = file_system::basename(input_path);

                            if (argc >= 4) {
                                auto str = file_system::normalize_slash(argv[3]);
                                if (file_system::is_folder(str) || str.back() == '/') {
                                    local_dir = str;
                                    if (local_dir.back() == '/') {
                                        local_dir.pop_back();
                                    }
                                }
                                else {
                                    local_dir = file_system::dirname(str);
                                    local_name = file_system::basename(str);
                                }
                            }

                            file_system::mkdir(local_dir);

                            // Prevents path from being treated as `//xxx'. On Windows, it's a network path.
                            auto local_path = local_dir.back() == '/' ? local_dir + local_name : local_dir + '/' + local_name;

                            stream::file_ostream fos;
                            if (fos.open(local_path)) {
                                std::cout << "Downloading `" << input_path << "' ...";
                                bkt.get_object(input_path, fos);
                                std::cout << " Done." << std::endl;
                            }
                            else {
                                std::cerr << "Error: Cannot open `" << local_path << "' for writing. "
                                    << "Failed to download `" << input_path << "'." << std::endl;
                            }

                        }
                        else {
                            auto local_dir = std::string(".");

                            if (argc >= 4) {
                                auto str = file_system::normalize_slash(argv[3]);
                                if (file_system::is_file(str)) {
                                    std::cerr << str << " exists, and is a file, aborting." << std::endl;
                                    return 1;
                                }
                                local_dir = str;
                                if (local_dir.back() == '/') {
                                    local_dir.pop_back();
                                }
                            }

                            std::vector<meta::content> files;
                            std::vector<std::string> folders;

                            bkt.list_objects(input_dir, true, &files, &folders);

                            auto prefix = input_dir.back() == '/' ? input_dir : input_dir + '/';

                            std::cout << "Summary: "
                                << folders.size() << (folders.size() > 1 ? " folders" : " folder")
                                << ", "
                                << files.size() << (files.size() > 1 ? " files" : " file")
                                << "."
                                << std::endl;

                            if (!folders.empty()) {
                                std::cout << std::endl;
                                for (const auto& f : folders) {
                                    auto path = local_dir + '/' + (f.c_str() + prefix.size());
                                    std::cout << "  Making directory `" << path << "' ...";
                                    file_system::mkdir(path);
                                    std::cout << " Done." << std::endl;
                                }
                            }

                            if (!files.empty()) {
                                std::cout << std::endl;
                                for (const auto& f : files) {
                                    stream::file_ostream fos;
                                    auto path = local_dir + '/' + (f.key().c_str() + prefix.size());
                                    if (fos.open(path)) {
                                        std::cout << "  Downloading `" << f.key() << "' ...";
                                        bkt.get_object(f.key(), fos);
                                        std::cout << " Done." << std::endl;
                                    }
                                    else {
                                        std::cerr << " Error: Cannot open `" << path << "' for writing. "
                                            << "Failed to download `" << f.key() << "'." << std::endl;
                                    }
                                }
                            }
                        }

                    }
                }
                else if (command == "upload") {
                    if (argc >= 4) {
                        auto dst = file_system::normalize_slash(argv[2]);
                        if (dst[0] != '/')  {
                            std::cerr << "Bad remote path: " << dst << std::endl;
                            return -1;
                        }

                        auto src = file_system::normalize_slash(argv[3]);

                        if (!file_system::is_file(src) && !file_system::is_folder(src)) {
                            std::cerr << "No such file or directory: " << src << std::endl;
                            return -1;
                        }

                        if (file_system::is_file(src)) {
                            std::string remote_path;

                            auto is_dst_folder = dst.back() == '/';

                            if (is_dst_folder) {
                                std::string remote_dir, remote_name;
                                remote_dir = dst;
                                remote_dir.pop_back();
                                remote_name = file_system::basename(src);
                                remote_path = remote_dir + '/' + remote_name;
                            }
                            else {
                                remote_path = dst;
                            }

                            stream::file_istream fis;
                            if (fis.open(src)) {
                                std::cout << "Uploading `" << src << "' ...";
                                bkt.put_object(remote_path, fis);
                                std::cout << " Done." << std::endl;
                            }
                            else {
                                std::cerr << "Error: Cannot open `" << src << "' for reading. Failed to upload." << std::endl;
                            }
                        }
                        else if (file_system::is_folder(src)) {
                            std::vector<std::string> files;

                            if (dst.back() != '/') {
                                dst += '/';
                            }

                            file_system::ls_files(src, &files);

                            std::cout << "Summary: " << files.size()
                                << (files.size() > 1 ? " files" : " file")
                                << " will be uploaded.\n"
                                << std::endl;

                            for (const auto& file : files) {
                                stream::file_istream fis;
                                auto path = src + '/' + file;
                                if (fis.open(path)) {
                                    std::cout << "  Uploading `" << file << "' ...";
                                    bkt.put_object(dst + file, fis);
                                    std::cout << " Done." << std::endl;
                                }
                                else {
                                    std::cerr << "  Error: Cannot open `" << path << "' for reading. Failed to upload." << std::endl;
                                }
                            }
                        }
                    }
                }
                else if (command == "delete") {
                    if (argc >= 3) {
                        auto spec = file_system::normalize_slash(argv[2]);
                        std::vector<meta::content> files;
                        std::vector<std::string> folders;

                        bkt.list_objects(spec, &files, &folders);

                        if (find_file(files, spec)) {
                            bkt.delete_object(spec);
                            std::cout << "Deleted." << std::endl;
                            return  0;
                        }

                        auto spec_back = spec;
                        if (spec.back() != '/') spec += '/';

                        if (find_folder(folders, spec)) {
                            for (const auto& file : files) {
                                if (std::strncmp(file.key().c_str(), spec.c_str(), spec.size()) == 0) {
                                    std::cout << "Deleting `" << file.key() << "' ...";
                                    bkt.delete_object(file.key());
                                    std::cout << " Done." << std::endl;
                                }
                            }

                            std::cout << "Deleting `" << spec << "' ...";
                            bkt.delete_object(spec);
                            std::cout << " Done." << std::endl;

                            return 0;
                        }

                        std::cerr << "No such file or directory: `" << spec_back << "'." << std::endl;
                        return -1;
                    }
                }
            }
        }
    }
    catch (osserr& e){
        std::cerr << e << std::endl;
        return -1;
    }
    catch (socket::socketexcept& e){
        e.push_stack(__FUNCTION__);
        socketerror_stderr_dumper(e);
        return -1;
    }
    catch (const char* e) {
        std::cerr << e << std::endl;
        return -1;
    }
    catch (...){
        std::cerr << "Fatal error: unhandled exception occurred!\n";
        return -1;
    }

    return 0;
}

static int repl()
{
    help();

    std::string prompt("$ ");
    std::string line;
    std::string prefix;

    while (std::cout << prompt && std::getline(std::cin, line)) {
        if (line[0] == '!') {
            ::_wsystem(strutil::from_utf8(line).c_str()+1);
            continue;
        }

        snippets::Argcv v;

        if (v.parse((prefix + " " + line).c_str())) {
            if (prefix.empty() && v.argc() == 1 && strcmp(v.argv()[0], "quit") == 0) {
                return 0;
            }

            if (!prefix.empty() && v.argc() == 1) {
                prompt = "$ ";
                prefix = "";
                continue;
            }

            if (v.argc() == 1) {
                prefix = v.argv()[0];
                prompt = prefix + " $ ";
                continue;
            }

            eval(v.argc(), v.argv());
        }
        else {
            std::cerr << "Bad arguments." << std::endl;
        }
    }

    return 0;
}

static int __main(int argc, const char* argv[])
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

	signal(SIGINT, [](int){});

#ifdef _WIN32
	socket::wsa wsa;
#endif

	try{
#ifdef _WIN32
		wsa.init();
#endif
	}
	catch (socket::socketexcept& e){
		socketerror_stderr_dumper(e);
		return 1;
	}

    // initialize key/secret
	try{
		read_access_key(&g_key);
	}
	catch (std::string& e){
		std::cerr << e << std::endl;
		return -1;
	}

    // initialize bucket configuration
    try{
        const char* val;

        val = std::getenv(OSS_BUCKET_NAME_ENV);
        if (val == nullptr) throw "Error: " OSS_BUCKET_NAME_ENV " not set.";
        g_oss_bucket_name = val;

        val = std::getenv(OSS_BUCKET_LOCATION_ENV);
        if (val == nullptr) throw "Error: " OSS_BUCKET_LOCATION_ENV " not set.";
        g_oss_bucket_location = val;
    }
    catch (const char* e) {
        std::cerr << e << std::endl;
        return -1;
    }

    return argc > 1
        ? eval(--argc, ++argv)
        : repl()
        ;
}

#ifdef _WIN32
int wmain(int argc, const wchar_t* _argv[])
{
#ifdef _DEBUG
    if (file_system::is_file("./__debug__")) {
        ::MessageBoxW(nullptr, L"Attach me", L"", MB_OK);
    }
#endif

    auto sarr = std::make_unique<std::string[]>(argc);
    auto argv = std::make_unique<const char*[]>(argc + 1);

    for(int i = 0; i < argc; i++) {
        sarr[i] = strutil::to_utf8(_argv[i]);
        argv[i] = sarr[i].c_str();
    }

    argv[argc] = nullptr;

    return __main(argc, argv.get());
}
#endif
