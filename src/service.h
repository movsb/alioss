/*-----------------------------------------------
File	: service.h
Date	: Nov 6, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#include <stdint.h>

#include <string>
#include <vector>

#include "socket.h"
#include "ossmeta.h"
#include "sign.h"

namespace alioss {

namespace service{

class service : protected set_endpoint_base
{
public:
	service(const accesskey& key)
		: _key(key)
    {}

    void set_endpoint(const std::string& host);

    bool connect();
    bool disconnect();
	
	void list_buckets(std::vector<meta::bucket>* buckets);

protected:
	meta::owner _owner;

protected:
	const accesskey&	_key;
	http::http	_http;
};

} // namespace service

} // namespace alioss

