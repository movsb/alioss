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

	// Delete this bucket
	bool delete_bucket();

	// Create bucket from _bkt
	bool create_bucket();

	// List specified object(s)
	// folder: "" or "folder_name/"
	bool list_objects(const char* folder="", bool recursive=false);

	/// dump objects & dump directories
	/// return false to interrupt iteration
	void dump_objects(std::function<bool(int i, const meta::content& object)> dumper);
	void dump_folders(std::function<bool(int i, const std::string& folder)> dumper);

	const std::vector<meta::content>& contents() { return _contents; }
	const std::vector<std::string>& folders() { return _common_prefixes; }

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
