/*-----------------------------------------------
File	: bucket
Date	: Nov 8, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#include <string>
#include <vector>
#include <functional>

#include "socket.h"
#include "ossmeta.h"
#include "osserror.h"

namespace alioss {

namespace bucket{

class bucket{
public:
	bucket(
		const accesskey& key,
		const std::string& name,
		const std::string& domain,
		const socket::endpoint& ep
		)
		: _key(key)
	{
        _name = name;
        _domain = domain;
		_endpoint = ep;
	}

	bool connect();
	bool disconnect();

	void list_objects(const std::string& folder, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders);

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

protected: // Request Parameters
	std::string _delimiter;
	std::string _marker;
	std::string _max_keys;
	std::string _prefix;

protected: // shared objects
	const accesskey& _key;

protected:
	http::http _http;
	socket::endpoint _endpoint;
    std::string _name;
    std::string _domain;
};

} // namespace bucket

} // namespace alioss
