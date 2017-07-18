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

class bucket : protected set_endpoint_base
{
public:
	bucket(const accesskey& key)
		: _key(key)
	{}

    void set_endpoint(const std::string& bucket_name, const std::string& bucket_location);

	bool connect();
	bool disconnect();

	void list_objects(const std::string& prefix, std::vector<meta::content>* objects, std::vector<std::string>* folders);
	void list_objects(const std::string& folder, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders);

    bool get_object(const std::string& obj, stream::ostream& os, http::getter getter = nullptr,
		const std::string& range="", const std::string& unmodified_since="");

	void delete_object(const std::string& obj);

	bool put_object(
        const std::string& obj,
        stream::istream& is,
        http::putter putter = nullptr
    );

	const http::header::head& head_object(const std::string& obj);

protected:
	void _list_objects_loop(const std::string& prefix, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders);
    bool _list_objects_internal(const std::string & prefix, const std::string & marker, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders, std::string * next_marker, std::unordered_set<std::string>* prefixes);

protected: // shared objects
	const accesskey& _key;

protected:
	http::http _http;
    std::string _name;
    std::string _location;
    std::string _host;
};

} // namespace bucket

} // namespace alioss
