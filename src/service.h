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
	
	// Verify user key
	// Returns:
	//	1. InvalidAccessKeyID - ID not exists
	//  2. Forbidden - ID exists, but secret wrong
	bool verify_user();

	bool list_buckets();

	const std::vector<meta::bucket>& buckets() { return _buckets; }
	void dump_buckets(std::function<void(int i, const meta::bucket& bkt)> dumper);

private:
	// avoid copy-ctor on vector::push_back()
	meta::bucket& bucket_create() {
		_buckets.push_back(meta::bucket());
		return _buckets[_buckets.size() - 1];
	}

// protected:
// 	std::string		_prefix;
// 	std::string		_marker;
// 	std::string		_max_keys;
// 	bool			_is_truncated;
// 	std::string		_next_marker;

protected:
	meta::owner _owner;
	std::vector<meta::bucket> _buckets;

protected:
	const accesskey&	_key;
	const socket::endpoint& _endpoint;
	http::http	_http;
};

} // namespace service

} // namespace alioss

