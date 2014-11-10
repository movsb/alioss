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
	service()
	{}
	
	bool connect();
	bool disconnect();

	void set_key(const char* id, const char* secret){
		_key.set_key(id, secret);
	}

	// Verify user key
	// Returns:
	//	1. InvalidAccessKeyID - ID not exists
	//  2. Forbidden - ID exists, but secret wrong
	bool verify_user();

	bool list_buckets();

private:
	// avoid copy-ctor on vector::push_back()
	meta::bucket& bucket_create() {
		_buckets.push_back(meta::bucket());
		return _buckets[_buckets.size() - 1];
	}

protected:
	std::string		_prefix;
	std::string		_marker;
	std::string		_max_keys;
	bool			_is_truncated;
	std::string		_next_marker;

protected:
	meta::owner _owner;
	std::vector<meta::bucket> _buckets;

protected:
	accesskey	_key;
	http::http	_http;
};

} // namespace service

} // namespace alioss

