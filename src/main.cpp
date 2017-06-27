#include <string>
#include <vector>
#include <cstdio>
#include <csignal>
#include <ctime>
#include <iostream>
#include <fstream>
#include <functional>
#include <regex>

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

using namespace alioss;

bool read_access_key(accesskey* key)
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

template<typename Element>
struct Finder
{
    template<typename T, typename R = std::string>
    struct ElementGetter
    {
        const R& operator()(const T& t)
        {
            return t;
        }
    };

    template<>
    struct ElementGetter<meta::content>
    {
        const std::string& operator()(const meta::content& file)
        {
            return file.key();
        }
    };

    typedef std::vector<Element> Container;
    typedef ElementGetter<Element> Getter;

    const Container&_con;

    Finder(const Container& con)
        : _con(con)
    {}

    bool operator()(const std::string& subject, bool case_sensitive)
    {
        return std::find_if(_con.cbegin(), _con.cend(), [&subject, &case_sensitive](const Element& search){
            return (case_sensitive ? strcmp : _stricmp)(subject.c_str(), Getter()(search).c_str()) == 0;
        }) != _con.cend();
    }
};

typedef Finder<std::string> FolderFinder;
typedef Finder<meta::content> FileFinder;

#ifdef _DEBUG
int main()
#else
int main(int argc, const char* argv[])
#endif // _DEBUG
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

	signal(SIGINT, [](int){});

#ifdef _WIN32
	socket::wsa wsa;
#endif
	socket::resolver resolver;

	try{
#ifdef _WIN32
		wsa.init();
#endif
		resolver.resolve("oss.aliyuncs.com", "http");
	}
	catch (socket::socketexcept& e){
		socketerror_stderr_dumper(e);
		return 1;
	}

	accesskey key;
	try{
		read_access_key(&key);
	}
	catch (std::string& e){
		std::cerr << e << std::endl;
		return -1;
	}

	socket::endpoint ep;
	ep.set_ep(resolver[0], 80);

	try {
		std::string str;

		service::service svc(key, ep);

		auto la_command_service = [&](){
            std::cout << "alioss - a simple Ali OSS manager\n";
            std::cout << "\n";
			std::cout 
                << "Syntax:\n\n"
                << "    <type>   <command>    [parameters...]\n\n"
                << "    bucket   list\n"
                << "\n"
                << "    object   list         <directory>\n"
                << "    object   head         <file>\n"
                << "    object   sign         <file>          <expiration>\n"
                << "\n"
                << "    object   download     <file>          [file/directory]\n"
                << "    object   download     <directory>     [directory]\n"
                << "\n"
                << "    object   upload       <file>          <file>\n"
                << "    object   upload       <directory>     <file>\n"
                << "    object   upload       <directory>     <directory>\n"
                << "\n"
				;
		};

#ifdef _DEBUG
        int argc = 4;
        const char* _argv[] = {
            "alioss.exe",
            "object",
            "list",
            "/"
        };

        const char** argv = _argv;
#endif // _DEBUG

        if(argc < 2) {
            la_command_service();
            return 0;
        }

        argc--;
        argv++;

        auto object = std::string(argv[0]);
        if(object == "bucket") {
            if(argc >= 2) {
                auto command = std::string(argv[1]);
                if(command == "list") {
                    std::vector<meta::bucket> buckets;
                    svc.list_buckets(&buckets);
                    for (const auto& bkt : buckets) {
                        std::cout
                            << "Name          : " << bkt.name() << std::endl
                            << "Location      : " << bkt.location() << std::endl
                            << "Creation Date : " << bkt.creation_date() << std::endl
                            << "End Point     : " << (bkt.location() + ".aliyuncs.com") << std::endl
                            << "Public Host   : " << (bkt.name() + "." + bkt.location() + ".aliyuncs.com") << std::endl
                            << std::endl
                        ;
                    }
                }
            }
        }
        else if(object == "object") {
            socket::endpoint ep;
            ep.set_ep("120.55.35.17", 80);
            bucket::bucket bkt(key, "twofei-wordpress", "twofei-wordpress.oss-cn-hangzhou.aliyuncs.com", ep);
            if(argc >= 2) {
                auto command = std::string(argv[1]);
                if(command == "list") {
                    if(argc >= 3) {
                        auto folder = std::string(argv[2]);
                        std::vector<meta::content> files;
                        std::vector<std::string> folders;

                        bkt.list_objects(folder, false, &files, &folders);

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
                else if(command == "head") {
                    if(argc >= 3) {
                        auto file = argv[2];
                        auto head = bkt.head_object(file);
                        head.dump([](size_t i, const std::string& key, const std::string& val){
                            std::printf("%-25s: %s\n", key.c_str(), val.c_str());
                            return true;
                        });
                    }
                }
                else if (command == "sign") {
                    if (argc >= 4) {
                        auto file = argv[2];

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

                                switch (unit)
                                {
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

                        auto expr= parse_expiration(argv[3]);
                        if (expr == -1) {
                            std::cerr << "Bad expiration." << std::endl;
                            return - 1;
                        }

                        auto current = std::time(nullptr);
                        auto expiration = current + expr;
                        auto expr_str = std::to_string(expiration);

                        std::string url;
                        url += "http://";
                        url += "twofei-wordpress.oss-cn-hangzhou.aliyuncs.com";

                        std::map<std::string, std::string> query = {
                            { "OSSAccessKeyId",  key.key()},
                            { "Expires",        expr_str },
                            { "Signature",      sign_url(key, expiration, std::string("/twofei-wordpress") + file)},
                        };

                        url += strutil::make_uri(file, query);

                        std::cout << url << std::endl;
                    }
                }
                else if(command == "download") {
                    if(argc >= 3) {
                        auto remote_path = file_system::normalize_slash(argv[2]);

                        std::vector<meta::content> files;
                        std::vector<std::string> folders;
                        bkt.list_objects(remote_path, &files, &folders);

                        bool is_download_file = remote_path.back() != '/';

                        if (is_download_file) {
                            auto has_file = FileFinder(files)(remote_path, false);
                            if (!has_file) {
                                has_file = FileFinder(files)(remote_path, true);
                                if (!has_file) {
                                    is_download_file = false;
                                }
                            }
                        }

                        if (!is_download_file) {
                            auto remote_path2 = remote_path.back() == '/' ? remote_path : remote_path + '/';
                            auto has_dir = remote_path2 == "/" || FolderFinder(folders)(remote_path2, false);
                            if (!has_dir) {
                                has_dir = FolderFinder(folders)(remote_path2, true);
                                if (!has_dir) {
                                    std::cerr << "No such file or folder: " << remote_path << std::endl;
                                    return 1;
                                }
                            }
                        }

                        if (is_download_file) {
                            auto local_dir = std::string(".");
                            auto local_name = file_system::basename(remote_path);

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

                            stream::file_ostream fos;
                            fos.open(local_dir + '/' + local_name);

                            std::cout << "Downloading " << remote_path << " ..." << std::endl;
                            bkt.get_object(remote_path, fos);
                        }
                        else {
                            auto local_dir = std::string(".");

                            if (argc >= 4) {
                                auto str = file_system::normalize_slash(argv[3]);
                                if (file_system::is_file(str)) {
                                    std::cerr << str << " exists, and is a folder." << std::endl;
                                    return 1;
                                }
                                local_dir = str;
                                if (local_dir.back() == '/') {
                                    local_dir.pop_back();
                                }
                            }

                            std::vector<meta::content> files;
                            std::vector<std::string> folders;

                            bkt.list_objects(remote_path, true, &files, &folders);

                            auto prefix = remote_path.back() == '/' ? remote_path : remote_path + '/';

                            for (const auto& f : folders) {
                                auto path = local_dir + '/' + (f.c_str() + prefix.size());
                                file_system::mkdir(path);
                            }

                            for (const auto& f : files) {
                                stream::file_ostream fos;
                                auto path = local_dir + '/' + (f.key().c_str() + prefix.size());
                                fos.open(path);
                                std::cout << "Downloading " << f.key() << " ..." << std::endl;
                                bkt.get_object(f.key(), fos);
                            }
                        }

                    }
                }
            }
        }
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		ossexcept_stderr_dumper(e);
		return -1;
	}
	catch (socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		socketerror_stderr_dumper(e);
	}
	catch (...){
		std::cerr << "Fatal error: unhandled exception occurred!\n";
	}

	return 0;
}

