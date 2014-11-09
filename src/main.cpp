#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "osserror.h"
#include "service.h"
#include "bucket.h"
#include "misc/color_term.h"

using namespace alioss;

int main()
{
	socket::wsa_instance wsa;

	meta::bucket mbkt;
	mbkt.set_name("twofei");
	mbkt.set_location("oss-cn-hangzhou");

	accesskey key;
	key.set_key("GADlpO6YWiTjXpYr", "42jHJIeiasJFIEjfwLKfE4uOxhacZbiferMYn8nADQygd4");

	socket::resolver resolver;
	resolver.resolve("oss.aliyuncs.com", "http");

	socket::endpoint ep;
	ep.set_ep(resolver[0].c_str(), 80);

	bucket::bucket bkt(key, mbkt, ep);

	try {
		color_term::color_term cterm;

		bkt.list_objects();

		std::cout << cterm(7, 3) << "--->Dumping folders:" << cterm(-1, -1) << std::endl;
		bkt.dump_folders([&](int i, const std::string& f){
			std::cout << "\t" << cterm(5, -1) << i << cterm(-1,-1) << ": " << f << std::endl;
			return true;
		});

		std::cout << cterm(7, 3) << "--->Dumping objects:" << cterm(-1, -1) << std::endl;
		bkt.dump_objects([&](int i, const meta::content& obj){
			std::cout << "\t" << cterm(5, -1) << i << cterm(-1, -1) << ": "
				<< obj.key() << "," << obj.size() << "B" << std::endl;
			return true;
		});

		cterm.restore();
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		ossexcept_stderr_dumper(e);
	}

	return 0;
}

