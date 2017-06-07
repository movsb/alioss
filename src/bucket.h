/*-----------------------------------------------
File	: bucket
Date	: Nov 8, 2014
Author	: twofei <anhbk@qq.com>
-----------------------------------------------*/

#include <string>
#include <sstream>
#include <vector>
#include <functional>

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

	bool list_objects(const std::string& folder, bool recursive=false);

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

protected:
	meta::content& create_content(){
		_contents.push_back(meta::content());
		return _contents[_contents.size() - 1];
	}

	std::string& create_common_prefix(){
		_common_prefixes.push_back(std::string());
		return _common_prefixes[_common_prefixes.size() - 1];
	}

protected: // Request Parameters
	std::string _delimiter;
	std::string _marker;
	std::string _max_keys;
	std::string _prefix;

protected: // shared objects
	const accesskey& _key;

protected: // this' objects
	std::vector<meta::content> _contents;
	std::vector<std::string>   _common_prefixes;

protected:
	http::http _http;
	socket::endpoint _endpoint;
    std::string _name;
    std::string _domain;
};

} // namespace bucket

} // namespace alioss
