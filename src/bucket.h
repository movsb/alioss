/*-----------------------------------------------
File	: bucket
Date	: Nov 8, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#include <string>
#include <sstream>
#include <vector>

#include "socket.h"
#include "accesskey.h"
#include "ossmeta.h"

namespace alioss {

namespace bucket{

class bucket{
public:
	bucket(
		const accesskey& key,
		const meta::bucket& bkt,
		const socket::endpoint& ep
		)
		: _key(key)
		, _bkt(bkt)
	{
		_endpoint = ep;
	}

	bool connect();
	bool disconnect();

	bool query();

	bool delete_();

protected:
	bool request();
	bool response();

	bool parse_response_body(const char* data, int size);

protected: // Request Parameters
	std::string _delimiter;
	std::string _marker;
	std::string _max_keys;
	std::string _prefix;

protected: // shared objects
	const accesskey& _key;
	const meta::bucket& _bkt;

protected: // this' objects
	std::vector<meta::content> _contents;
	std::vector<std::string>   _common_prefixes;

protected:
	http::http _http;
	socket::endpoint _endpoint;
};

} // namespace bucket

} // namespace alioss
