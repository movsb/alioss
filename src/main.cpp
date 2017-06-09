#include <string>
#include <vector>
#include <cstdio>
#include <csignal>
#include <iostream>
#include <sstream>
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
#include "misc/os.hpp"
#include "misc/time.h"
#include "misc/dirmgr.h"

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

static std::string basename(const std::string& file)
{
    auto off = file.rfind('/');
    if(off != file.npos) {
        return file.substr(off + 1);
    }
    else {
        return file;
    }
}

static bool is_folder(const std::string& path)
{
#ifdef _WIN32
    DWORD dwAttr = ::GetFileAttributes(path.c_str());
    return dwAttr & FILE_ATTRIBUTE_DIRECTORY;
#else
    assert(0);
#endif
}

//int main(int argc, const char* argv[])
int main()
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
		std::cout << "socket_init ..." << std::endl;
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
            std::cout << "alioss - the simple Ali OSS manager\n";
            std::cout << "\n";
			std::cout 
                << "    bucket list\n"
                << "\n"
                << "    object list         <directory>\n"
                << "    object head         <file>\n"
                << "\n"
                << "    object download     <file>          [file/directory]\n"
                << "    object download     <directory>     [directory]\n"
                << "\n"
                << "    object upload       <file>          <file>\n"
                << "    object upload       <directory>     <file>\n"
                << "    object upload       <directory>     <directory>\n"
                << "\n"
				;
		};

        int argc = 4;
        const char* _argv[] = {
            "alioss.exe",
            "object",
            "list",
            "/e"
        };

        const char** argv = _argv;

        if(argc < 2) {
            la_command_service();
            return 0;
        }

        argc--;
        argv++;

        auto object = std::string(argv[0]);
        if(object == "bucket") {
            if(argc == 2) {
                auto command = std::string(argv[1]);
                if(command == "list") {
                    std::vector<meta::bucket> buckets;
                    svc.list_buckets(&buckets);
                }
            }
        }
        else if(object == "object") {
            socket::endpoint ep;
            ep.set_ep("120.77.166.164", 80);
            bucket::bucket bkt(key, "twofei-test", "twofei-test.oss-cn-shenzhen.aliyuncs.com", ep);
            if(argc >= 2) {
                auto command = std::string(argv[1]);
                if(command == "list") {
                    if(argc >= 3) {
                        auto folder = std::string(argv[2]);
                        bkt.list_objects(folder, true);
                    }
                }
                else if(command == "head") {
                    if(argc >= 3) {
                        auto file = argv[2];
                        auto head = bkt.head_object(file);
                    }
                }
                else if(command == "download") {
                    if(argc >= 3) {
                        auto remote_path = argv[2];
                        auto remote_name = basename(remote_path);
                        auto local_dir = std::string(".");
                        auto local_name = remote_name;

                        if(argc >= 4) {
                            if(is_folder(argv[3])) {
                                local_dir = std::string(argv[3]);
                            }
                        }

                        stream::file_ostream os;
                        os.open(local_dir + "/" + local_name);

                        bkt.get_object(remote_path, os);
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

