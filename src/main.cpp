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
	mbkt.set_name("twofei2");
	mbkt.set_location("oss-cn-hangzhou");

	accesskey key;
	key.set_key("GADlpO6YWiTjXpYr", "YF42jHE4uOxhacZbiferMYn8nADQygd4");

	socket::resolver resolver;
	resolver.resolve("oss.aliyuncs.com", "http");

	socket::endpoint ep;
	ep.set_ep(resolver[0].c_str(), 80);

	bucket::bucket bkt(key, mbkt, ep);

	try {
		color_term::color_term cterm;

		http::str_body_ostream bs;
		object::object obj(key, mbkt, ep);

		class str_istream :public stream::istream{
		public:
			explicit str_istream(const std::string& str)
				: _str(str), _pos(0)
			{}

			virtual int read_some(unsigned char* buf, int sz){
				sz = sz > size() ? size() : sz;
				::memcpy(buf, _str.c_str() + _pos, sz);
				_pos += sz;
				return sz;
			}

			virtual int size() const{
				return _str.size()-_pos;
			}
		protected:
			int _pos;
			const std::string& _str;
		};

		std::string str("text_content");
		str_istream si(str);
		obj.put_object("ddd", si);

		cterm.restore();
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		ossexcept_stderr_dumper(e);
	}

	return 0;
}

