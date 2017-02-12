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
#include "object.h"
#include "ossmeta.h"
#include "misc/color_term.h"
#include "misc/strutil.h"
#include "misc/stream.h"
#include "misc/os.hpp"
#include "misc/time.h"

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

int exec_sys_cmd(const char* cmd, const char* arg="")
{
	if (!cmd || !*cmd) return 0;

	std::string s(cmd);

#ifdef _WIN32
    // windows 上执行cd不起作用
    if(s == "cd" && arg && *arg) {
        if(_chdir(arg) != 0)
            std::cerr << "failed to chdir call\n";
        return -1;
    }
#endif

	s += " ";
	s += arg;
	return system(s.c_str());
}

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
		for (int i = 0; i < (int)resolver.size(); i++){
			std::cout << "  " << resolver[i] << std::endl;
		}
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
	ep.set_ep(resolver[0].c_str(), 80);

	try {
		color_term::color_term cterm;
		std::string str;

		std::cout << "Logging ... ";

		service::service svc(key, ep);
		svc.verify_user();
		std::cout << cterm(2,-1) << "Success!" << cterm(-1,-1) << std::endl;

		auto la_command_service = [&](){
			std::cout << cterm(2, -1) << "Commands for: " << "Service" << cterm(-1, -1) << std::endl;
			std::cout 
				<< "    list                list buckets\n"
				<< "    create...           create new bucket\n"
				<< "    del <id>            delete bucket by id <id>\n"
				<< "    enter <id>          enter some bucket\n"
				<< "    quit                quit program\n"
				<< "    help                display help text\n"
				;
		};

		la_command_service();

		for (;;){

			std::string cmd, arg;
			read_command(&cmd, &arg, "service");

			if (cmd[0] == '!'){
				exec_sys_cmd(&cmd[1], arg.c_str());
				continue;
			}

			const char* service_cmds[] = {
				"list",
				"create",
				"del",
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
				std::string thecmd(service_cmds[match.back()]);
				if (thecmd == "create"){
					std::cout << cterm(2, -1);
					printf("    %-4s%-16s%s","ID","Location","Location-Alias");
					std::cout << cterm(-1, -1) << std::endl;

					for (int i = 0; i<meta::oss_data_center_count; i++){
						auto& dc = meta::oss_data_center[i];
						printf("    %-4d%-16s%s\n", i + 1, dc.name, dc.location);
					}
					std::cout << std::endl;

					int id;
					std::string name;

					for (const char* prompt = "Please input location-id & new bucket-name: ";;){
						std::string line;
						std::cout << prompt;
						std::getline(std::cin, line, '\n');
						if (line.size() == 0) goto next_cmd;

						std::stringstream liness(line);
						if (liness >> id >> name){
							if (id<1 || id>meta::oss_data_center_count){
								std::cout << cterm(4, -1) << "invalid location id - " << id << ", "
									<< "range: " << 1 << "-" << meta::oss_data_center_count
									<< cterm(-1, -1) << std::endl;
								prompt = "Try again: ";
								continue;
							}

							std::regex re(R"(^[a-z0-9]{1}[a-z0-9\-]{1,61}[a-z0-9]{1}$)", std::regex_constants::egrep);
							if (!std::regex_match(name, re)){
								std::cout << cterm(4,-1)
									<< "Sorry, invalid bucket name, constraints are:" << cterm(-1, -1) << std::endl;
								std::cout
									<< "    1. can only contains: a-z, 0-9, -\n"
									<< "    2. must not start with or end with `-'\n"
									<< "    3. the length must be in range of 3-63\n"
									<< std::endl;
								prompt = "Try again: ";
								continue;
							}
							break;
						}
						else{
							prompt = "invalid input, try again: ";
						}
					}

					try{
						meta::bucket bkt(name.c_str(), meta::oss_data_center[id-1].location,"");
						bucket::bucket bucket(key, bkt, ep);
						bucket.create_bucket();
						std::cout << cterm(2, -1) << "Successfully created!" << cterm(-1, -1) << std::endl;
					}
					catch (ossexcept& e){
						ossexcept_stderr_dumper(e);
					}
				}
				else if(thecmd == "del"){
					int id = 0;
					if (arg.size() == 0 || (id = atoi(arg.c_str())) <= 0 || id > (int)svc.buckets().size()){
						std::cout << "del - Invalid bucket ID!\n";
						goto next_cmd;
					}

					try{
						meta::bucket bkt(svc.buckets()[id - 1].name().c_str(), svc.buckets()[id-1].location().c_str(),"");
						bucket::bucket bucket(key, bkt, ep);
						bucket.delete_bucket();
						std::cout << cterm(2, -1) << "Successfully deleted!" << cterm(-1, -1) << std::endl;
					}
					catch (ossexcept& e){
						ossexcept_stderr_dumper(e);
					}
				}
				else if (thecmd == "list"){
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
						std::cout << cterm(2, -1) << "Commands for: " << "Bucket" << cterm(-1, -1) << std::endl;
                        std::cout
							<< "    list            list objects in current directory\n"
							<< "    cd [dir]        change directory (no slash('/'), no recursion)\n"
							<< "    pwd             print working directory(oss', not system)\n"
							<< "    del <obj/dir>   delete object/folder in current directory\n"
							   "                    ignores non-exist object\n"
							<< "    mkdir <name>    create directory (need slash('/'))\n"
							<< "    head <obj>      show object meta info\n"
							<< "    down <obj>      download object to working directory<obj>\n"
							<< "    up <file>       upload files to current directory,\n"
                               "                    with a `/' suffix means to upload entire folder\n"
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
						"mkdir",
						"head",
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

						if (cmd[0] == '!'){
							exec_sys_cmd(&cmd[1], arg.c_str());
							continue;
						}

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
									for (int i = 0; i<(int)vs.size(); i++){
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
								bucket.list_objects(cur_dir.c_str());

								std::cout << cterm(7, 2) << "Folders:" << cterm(-1, -1) << std::endl;
								bucket.dump_folders([&](int i, const std::string& folder)->bool{
									std::cout << "    " << folder.c_str()+cur_dir.size()-1 << std::endl;
									return true;
								});

								std::cout << cterm(7, 2) << "Files:" << cterm(-1, -1) << std::endl;
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
								if (arg.size() == 0 || (arg.find('/')!=std::string::npos && arg.find('/') != arg.size() - 1)){
									std::cout << "Plz specify the object name you want to delete\n";
									goto next_bucket_cmd;
								}

								object::object object(key, bkt, ep);
								try{
									object.delete_object((cur_dir+arg).c_str());
								}
								catch (ossexcept& e){
									ossexcept_stderr_dumper(e);
								}
							}
							else if (thecmd == "mkdir"){
								if (arg.size() == 0 || arg.find('/') != arg.size() - 1){
									std::cout << "Plz correctly specify the folder name\n";
									goto next_bucket_cmd;
								}

								try{
									object::object object(key, bkt, ep);
									object.create_folder((cur_dir+arg).c_str());
								}
								catch (ossexcept& e){
									ossexcept_stderr_dumper(e);
								}
							}
							else if (thecmd == "head"){
								if (arg.size() == 0 || (arg.find('/') != std::string::npos && arg.find('/') != arg.size() - 1)){
									std::cout << "Plz specify the object name you want to head to\n";
									goto next_bucket_cmd;
								}

								try{
									object::object object(key, bkt, ep);
									object.head_object((cur_dir+arg).c_str()).dump([&](size_t i, const std::string& k, const std::string& v){
										printf("%-20s: %s\n", k.c_str(), v.c_str());
										return true;
									});
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
										return static_cast<int>(_fstm.width()); // ???
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
                                    progress pg;
                                    std::cout << "Downloading ... ";
                                    pg.start();
                                    object.get_object((cur_dir + arg).c_str(), fos, [&](const unsigned char* data, int size, int total, int transferred) {
                                        pg.set(int(float(transferred) / total * 100));
                                    });
                                    pg.end();
									std::cout << "completed.\n";
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

                                bool is_folder = arg.back() == '/' || arg.back() == '\\';

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

                                std::vector<std::string> files;
                                if(is_folder == false) {
                                    files.push_back(arg.c_str());
                                }
                                else {
                                    files.clear();
                                    os::ls_files(arg.c_str(), &files);
                                    if(files.size() == 0) {
                                        std::cerr << "    Your search doesn't match anything, nothing to be done.\n";
                                        goto next_bucket_cmd;
                                    }

                                    std::cout << "    " << files.size() << " file(s) will be uploaded.\n";
                                }

                                int uploaded = 0;
                                for(auto& file : files) {
                                    auto& arg = file;
                                    file_istream fis;
                                    if(!fis.open(arg.c_str())) {
                                        std::cout << "Cannot open file `" << arg << "' for reading!\n";
                                        continue;
                                    }

                                    object::object object(key, bkt, ep);
                                    try {
                                        progress pg;
                                        std::string obj = cur_dir + strutil::remove_relative_path_prefix(arg.c_str());
                                        std::cout << "Uploading `" << arg << "' ... ";
                                        pg.start();
                                        object.put_object(obj.c_str(), fis, [&](const unsigned char* data, int size, int total, int transferred) {
                                            pg.set(int(float(transferred) / total * 100));
                                        });
                                        pg.end();
                                        uploaded++;
                                        std::cout << "succeeded.\n";
                                    } catch(ossexcept& e) {
                                        ossexcept_stderr_dumper(e);
                                    }
                                }
                                std::cout << "\n    " << files.size() << " file(s) to upload, "
                                    << uploaded << " file(s) successfully uploaded.\n";
							}
						}
						else{
							std::cout << "Command not found: " << cterm(4, -1) << cmd << cterm(-1, -1);
							if (match.size()) std::cout << ", did you mean: \n";
							else std::cout << "\n";

							for (auto& m : match){
								std::cout << "\t" << bucket_cmds[m] << "\n";
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
	catch (socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		socketerror_stderr_dumper(e);
	}
	catch (...){
		std::cerr << "Fatal error: unhandled exception occurred!\n";
	}

	return 0;
}

