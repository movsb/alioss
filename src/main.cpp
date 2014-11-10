#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "osserror.h"
#include "service.h"
#include "bucket.h"
#include "object.h"
#include "misc/color_term.h"

using namespace alioss;

int main()
{
	socket::wsa_instance wsa;

	meta::bucket mbkt;
	mbkt.set_name("twofei32");
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

		service::service svc;
		svc.set_key(key.key().c_str(), key.secret().c_str());
		svc.list_buckets();

		cterm.restore();
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		ossexcept_stderr_dumper(e);
	}

	return 0;
}

