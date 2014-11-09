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
#include "osserror.h"

namespace alioss {

namespace bucket{

namespace bucket_error{
	class invalid_location_constraint : public osserr{
	public:
		invalid_location_constraint(void* d){
			parse(d);
		}

	protected:
		virtual void node_handler(void* n) override;
		virtual void dump(std::function<void(const std::string&)> dumper) override;

	protected:
		std::string _location;
	};

	class invalid_bucket_name : public osserr{
	public:
		invalid_bucket_name(void* d){
			parse(d);
		}

	protected:
		virtual void node_handler(void* n) override;
		virtual void dump(std::function<void(const std::string&)> dumper) override;

	protected:
		std::string _bucket_name;
	};
}

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

	// Delete bucket
	bool delete_bucket();

	// Create bucket
	bool create_bucket();


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
