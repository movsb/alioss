/*-----------------------------------------------
File	: service.h
Date	: Nov 6, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#include <stdint.h>

#include <string>
#include <sstream>
#include <vector>

#include "socket.h"
#include "accesskey.h"
#include "ossmeta.h"

namespace alioss {

namespace service{

class service {
public:
	service(const accesskey& key, const socket::endpoint& ep)
		: _key(key)
		, _endpoint(ep)
	{
	}

    bool connect();
    bool disconnect();
	
	void list_buckets(std::vector<meta::bucket>* buckets);

protected:
	meta::owner _owner;

protected:
	const accesskey&	_key;
	const socket::endpoint& _endpoint;
	http::http	_http;
};

} // namespace service

} // namespace alioss

