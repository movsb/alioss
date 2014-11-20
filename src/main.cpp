#include <string>
#include <vector>
#include <cstdio>
#include <csignal>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "osserror.h"
#include "service.h"
#include "bucket.h"
#include "object.h"
#include "misc/color_term.h"
#include "misc/strutil.h"
#include "misc/stream.h"

using namespace alioss;

bool read_command(std::string* cmd, std::string* arg, const char* prompt="")
{
	color_term::color_term cterm;
	std::string str;

next_cmd:
	std::cout << cterm(3, -1) << "alioss";
	if (prompt&&*prompt){
		std::cout << " " << prompt << "> ";
	}
	else{
		std::cout << "> ";
	}

	std::cout << cterm(-1, -1);

	std::getline(std::cin, str, '\n');
	str = strutil::strip(str);
	if (!str.size()) goto next_cmd;

	auto pos = str.find(' ');
	if (pos == std::string::npos)
		pos = str.find('\t');

	if (pos != std::string::npos){
		*cmd = str.substr(0, pos - 0);
		const char* p = &str[pos];
		while (*p && (*p == ' ' || *p == '\t'))
			++p;
		*arg = std::string(p);
	}
	else{
		*cmd = str;
		*arg = "";
	}

	cterm.restore();
	return true;
}

int main()
{
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
		std::cout << "resolving oss.aliyuncs.com ...";
		resolver.resolve("oss.aliyuncs.com", "http");

		std::cout << std::endl;
		for (int i = 0; i < resolver.size(); i++){
			std::cout << "  " << resolver[i] << std::endl;
		}
	}
	catch (socket::socketexcept& e){
		socketerror_stderr_dumper(e);
		return 1;
	}

// 	std::string keyid,keysec;
// 	std::cout << "Input access key ID: ";
// 	std::getline(std::cin, keyid, '\n');
// 	std::cout << "Input access key secret: ";
// 	std::getline(std::cin, keysec, '\n');

	accesskey key;
	//key.set_key(keyid.c_str(), keysec.c_str());
	key.set_key("GADlpO6YWiTjXpYr", "YF42jHE4uOxhacZbiferMYn8nADQygd4");

	socket::endpoint ep;
	ep.set_ep(resolver[0].c_str(), 80);

	try {
		color_term::color_term cterm;
		std::string str;

		std::cout << "Logging ... ";

		service::service svc(key, ep);
		svc.verify_user();
		std::cout << cterm(2,-1) << "Success!" << cterm(-1,-1) << std::endl;

		auto la_command_service = [&](){
			std::cout << cterm(2, -1) << "Commands for: " << "Service\n" << cterm(-1, -1);
			std::cout 
				<< "    list                list buckets\n"
				<< "    enter <id>          enter some bucket\n"
				<< "    quit                quit program\n"
				<< "    help                display help text\n"
				;
		};

		la_command_service();

		for (;;){

			std::string cmd, arg;
			read_command(&cmd, &arg, "service");

			const char* service_cmds[] = {
				"list",
				"enter",
				"quit",
				"help",
				nullptr
			};

			auto find_cmd = [](const char* a[], const char* c, std::vector<int>* match)->bool{
				bool found = false;
				match->clear();
				for (int i = 0; a[i]; i++){
					if (strstr(a[i], c) == a[i]){
						match->push_back(i);
						if (strlen(a[i]) == strlen(c)){
							found = true;
							break;
						}
					}
				}
				return found;
			};

			std::vector<int> match;
			if (find_cmd(service_cmds, cmd.c_str(), &match) || match.size()==1){
				std::string thecmd(service_cmds[match[match.size() - 1]]);
				if (thecmd == "list"){
					svc.list_buckets();
					svc.dump_buckets([&](int i, const meta::bucket& bkt){
						std::cout << cterm(3, -1) << i << cterm(-1, -1) << ": " << bkt.name() << std::endl;
						std::cout << "    Location    : " << bkt.location() << std::endl;
						std::cout << "    CreationDate: " << bkt.creation_date() << std::endl;
					});
				}
				else if (thecmd == "enter"){
					int id = 0;
					if (arg.size() == 0 || (id=atoi(arg.c_str())) <= 0 || id>(int)svc.buckets().size()){
						std::cout << "enter - Invalid bucket ID!\n";
						goto next_cmd;
					}

					auto la_command_bucket = [&](){
						std::cout << cterm(2, -1) << "Commands for: " << "Bucket\n" << cterm(-1, -1);
						std::cout
							<< "    list            list objects in current directory\n"
							<< "    cd [dir]        change directory (no slash('/'), no recursion)\n"
							<< "    pwd             print working directory\n"
							<< "    del <obj>       delete object in current directory, ignores non-exist object\n"
							<< "    down <obj>      download object to working directory<obj>\n"
							<< "    up <file>       upload files to current directory\n"
							<< "    svc             back to service command\n"
							<< "    quit            quit program\n"
							<< "    help            show this help message\n"
							;
					};
					const char* bucket_cmds[] = {
						"list",
						"cd",
						"pwd",
						"del",
						"down",
						"up",
						"svc",
						"quit",
						"help",
						nullptr
					};


					meta::bucket bkt(svc.buckets()[id-1].name().c_str(), svc.buckets()[id-1].location().c_str(), "");
					bucket::bucket bucket(key, bkt, ep);

					la_command_bucket();

					std::string cur_dir("/");

					for (;;){
						std::string cmd, arg;
						read_command(&cmd, &arg, "bucket");

						std::vector<int> match;
						if (find_cmd(bucket_cmds, cmd.c_str(), &match) || match.size() == 1){
							std::string thecmd(bucket_cmds[match[match.size() - 1]]);

							if (thecmd == "help"){
								la_command_bucket();
							}
							else if (thecmd == "quit"){
								std::cout << "quiting...\n";
								exit(0);
							}
							else if (thecmd == "pwd"){
								std::cout << cur_dir << std::endl;
							}
							else if (thecmd == "cd"){
								if (arg.size() == 0 || arg == "/"){
									cur_dir = "/";
									goto next_bucket_cmd;
								}

								if (arg == "."){
									goto next_bucket_cmd;
								}
								else if (arg == ".."){
									if (cur_dir == "/"){
										goto next_bucket_cmd;
									}
									else{
										cur_dir = cur_dir.substr(0, cur_dir.rfind('/', cur_dir.size()-2)+1);
										goto next_bucket_cmd;
									}
								}

								auto find_dirs = [](const std::vector<std::string>& vs, const std::string& name, std::vector<int>* match)->bool{
									bool found = false;
									match->clear();
									for (int i = 0; (int)i<vs.size(); i++){
										if (vs[i].find(name) == 0){
											match->push_back(i);
											if (vs[i].size() == name.size()+1){
												found = true;
												break;
											}
										}
									}
									return found;
								};

								std::vector<int> matchdirs;
								if (find_dirs(bucket.folders(), std::string(cur_dir.c_str()+1+arg), &matchdirs) || matchdirs.size() == 1){
									cur_dir += bucket.folders()[matchdirs.back()].c_str() + cur_dir.size()-1;
								}
								else{
									std::cout << "No such folder: " << cterm(4, -1) << arg << cterm(-1, -1);
									if (matchdirs.size()) std::cout << ", do you mean: \n";
									else std::cout << "\n";

									for (auto& m : matchdirs){
										std::cout << "\t" << bucket.folders()[m] << "\n";
									}
								}
							}
							else if (thecmd == "svc"){
								goto next_cmd;
							}
							else if (thecmd == "list"){
								bucket.list_objects(cur_dir.c_str()+1);

								std::cout << cterm(7, 2) << "Folders:\n" << cterm(-1, -1);
								bucket.dump_folders([&](int i, const std::string& folder)->bool{
									std::cout << "    " << folder.c_str()+cur_dir.size()-1 << std::endl;
									return true;
								});

								std::cout << cterm(7, 2) << "Files:\n" << cterm(-1, -1);
								bucket.dump_objects([&](int i, const meta::content& obj)->bool{
									if (obj.key().back() == '/'){
										// ignore current directory
									}
									else{
										std::cout << "    " << obj.key().c_str()+cur_dir.size()-1 << std::endl;
									}
									return true;
								});
							}
							else if (thecmd == "del"){
								if (arg.size() == 0 || arg.find('/') != std::string::npos){
									std::cout << "Plz specify the object name you want to delete\n";
									goto next_bucket_cmd;
								}

								object::object object(key, bkt, ep);
								try{
									object.delete_object(arg.c_str());
								}
								catch (ossexcept& e){
									ossexcept_stderr_dumper(e);
								}
							}
							else if (thecmd == "down"){
								if (arg.size() == 0 || arg.find('/') != std::string::npos){
									std::cout << "Plz correctly specify the object name\n";
									goto next_bucket_cmd;
								}

								class file_ostream : public stream::ostream{
								public:
									virtual int size() const override{
										return _fstm.width(); // ???
									}
									virtual int write_some(const unsigned char* buf, int sz) override{
										_fstm.write((char*)buf, sz);
										return sz;
									}

								public:
									~file_ostream(){
										close();
									}

									bool open(const char* file){
										_fstm.open(file, std::ios_base::ate | std::ios_base::binary);
										return _fstm.is_open();
									}

									void close() {
										_fstm.close();
									}

								protected:
									std::ofstream _fstm;
								};

								file_ostream fos;
								if (!fos.open(arg.c_str())){
									std::cout << "Cannot open file `" << arg << "' for writing\n";
									goto next_bucket_cmd;
								}

								object::object object(key, bkt, ep);
								try{
									object.get_object((cur_dir+arg).c_str()+1, fos);
									std::cout << "Download completed\n";
								}
								catch (ossexcept& e){
									ossexcept_stderr_dumper(e);
								}
							}
							else if (thecmd == "up"){
								if (arg.size() == 0 ){
									std::cout << "Plz correctly specify the file name you want to upload\n";
									goto next_bucket_cmd;
								}

								class file_istream : public stream::istream{
								public:
									virtual int size() const override{
										return _fsize;
									}
									virtual int read_some(unsigned char* buf, int sz) override{
										_filestm.read((char*)buf, sz);
										_fsize -= (int)_filestm.gcount();
										return (int)_filestm.gcount();
									}

								public:
									bool open(const char* file){
										_filestm.open(file, std::ios_base::binary);
										if (_filestm.is_open()){
											_filestm.seekg(0, std::ios_base::end);
											_fsize = (size_t)_filestm.tellg();
											_filestm.seekg(0, std::ios_base::beg);
										}
										return _filestm.is_open();
									}
									void close(){
										_filestm.close();
									}

									~file_istream(){
										close();
									}

								protected:
									size_t _fsize;
									std::ifstream _filestm;
								};

								file_istream fis;
								if (!fis.open(arg.c_str())){
									std::cout << "Cannot open file `" << arg << "' for reading!\n";
									goto next_bucket_cmd;
								}

								auto get_file_name = [](const std::string& arg)->std::string{
									auto slash = arg.rfind('/');
									auto backslash = arg.rfind('\\');

									auto real_slash = slash;
									if (slash != std::string::npos && backslash != std::string::npos){
										real_slash = slash > backslash ? slash : backslash;
									}
									else if (slash != std::string::npos){
										real_slash = slash;
									}
									else{
										real_slash = backslash;
									}

									return arg.substr(real_slash+1);
								};

								object::object object(key, bkt, ep);
								try{
									object.put_object(get_file_name(arg).c_str(), fis);
									std::cout << "Upload succeeded!\n";
								}
								catch (ossexcept& e){
									ossexcept_stderr_dumper(e);
								}
							}
						}
						else{
							std::cout << "Command not found: " << cterm(4, -1) << cmd << cterm(-1, -1);
							if (match.size()) std::cout << ", did you mean: \n";
							else std::cout << "\n";

							for (auto& m : match){
								std::cout << "\t" << service_cmds[m] << "\n";
							}
						}
					next_bucket_cmd:;
					}

				}
				else if (thecmd == "help"){
					la_command_service();
				}
				else if (thecmd == "quit"){
					std::cout << "quiting...\n";
					return 0;
				}
			}
			else{
				std::cout << "Command not found: " << cterm(4, -1) << cmd << cterm(-1, -1);
				if(match.size()) std::cout << ", did you mean: \n";
				else std::cout << "\n";

				for (auto& m : match){
					std::cout << "\t" << service_cmds[m] << "\n";
				}
			}
		next_cmd:;
		}

		cterm.restore();
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		ossexcept_stderr_dumper(e);
		return -1;
	}

	return 0;
}

