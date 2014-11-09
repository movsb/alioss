/*-----------------------------------------------
File	: object
Date	: Nov 9, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#ifndef __alioss_object_h__
#define __alioss_object_h__

#include <string>

#include "socket.h"
#include "ossmeta.h"
#include "accesskey.h"
#include "osserror.h"

namespace alioss {

namespace object {

	namespace object_error{
		// exactly the same as bucket_error::invalid_bucket_name
		class no_such_bucket : public osserr{
		public:
			no_such_bucket(const char* xml, int size){
				parse(xml, size);
			}
			no_such_bucket(void* d){
				parse(d);
			}

		protected:
			virtual void node_handler(void* n) override;
			virtual void dump(std::function<void(const std::string&)> dumper) override;

		protected:
			std::string _bucket_name;
		};
	}

class oject{
public:
	oject(
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

	bool delete_object(const char* obj);

protected: // shared objects
	const accesskey& _key;
	const meta::bucket& _bkt;

protected:
	http::http _http;
	socket::endpoint _endpoint;
};


} // namespace object

} // namespace alioss

#endif // !__alioss_object_h__

