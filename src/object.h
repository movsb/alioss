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

class object{
public:
	object(
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

	/*--------------------------------------------------------------------
	@desc Get(download) an object named `obj' from specified `range', and writes it to `os'
	@param obj: the name and the path of the object that you want to get.
		Must be url-encoded, and be UTF-8-ed.
		e.g.: /path/to/file.txt
	@param os: the ostream which get_object() writes data to.
	@param range: returns data from this range: [<start>-<end>], or leave it empty 
	@param unmodified_since: if `range', `'unmodified_since' checks whether the object
			has not been modified since that time, and if so, get operation is starting.
			else, get_object() throws object_error:: Precondition failed.
		it has GMT time format.
	--------------------------------------------------------------------*/
	bool get_object(const char* obj, stream::ostream& os, http::getter getter = nullptr,
		const std::string& range="", const std::string& unmodified_since="");

	bool delete_object(const std::string& obj);

	bool put_object(
        const std::string& obj,
        stream::istream& is,
        http::putter putter = nullptr,
        const char* content_type="",
        const char* content_disposition="",
        const char* content_encoding=""
    );

	bool create_folder(const std::string& name);

	const http::header::head& head_object(
		const std::string& obj,
		const char* if_modified_since = nullptr,
		const char* if_unmodified_since = nullptr,
		const char* if_match = nullptr,
		const char* if_none_match = nullptr
    );

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

