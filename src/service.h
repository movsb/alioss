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

namespace alioss {

namespace service{

class bucket{
public:
	bucket() {}

	bucket(const char* name,const char*	loc, const char* date)
		: _name(name)
		, _location(loc)
		, _creation_date(date)
	{}

	void set_name(const char* name) {
		_name = name;
	}
	const std::string& name() const {
		return _name;
	}

	void set_location(const char* loc){
		_location = loc;
	}
	const std::string& location() const {
		return _location;
	}
	
	void set_creation_date(const char* date){
		_creation_date = date;
	}
	const std::string& creation_date() const {
		return _creation_date;
	}

protected:
	std::string _name;
	std::string _location;
	std::string _creation_date;
};

class owner {
public:
	owner() {}
	owner(const char* id, const char* name)
		: _id(id)
		, _display_name(name)
	{}

	void set_id(const char* id) {
		_id = id;
	}

	void set_display_name(const char* name){
		_display_name = name;
	}

	const std::string& id() const {
		return _id;
	}

	const std::string& display_name() const {
		return _display_name;
	}

protected:
	std::string _id;
	std::string _display_name;
};

class service {
private:
	class body_stream : public stream::ostream {
	public:
		virtual int size() const override {
			return static_cast<int>(_buffer.size());
		}

		virtual int write_some(const unsigned char* buf, int sz) override
		{
			_buffer += std::string((char*)buf, sz);
			return sz;
		}

	public:
		unsigned char* data() {
			return (unsigned char*)_buffer.c_str();
		}

	protected:
		std::string _buffer;
	};

public:
	service()
	{}
	
	bool connect();
	bool disconnect();

	void set_key(const char* id, const char* secret){
		_key.set_key(id, secret);
	}

	bool query();

protected:
	bool request();
	bool response();

	bool parse_response_body(const char* data, int size);

private:
	// avoid copy-ctor on vector::push_back()
	bucket& bucket_create() {
		_buckets.push_back(bucket());
		return _buckets[_buckets.size() - 1];
	}

protected:
	std::string		_prefix;
	std::string		_marker;
	std::string		_max_keys;
	bool			_is_truncated;
	std::string		_next_marker;

protected:
	owner _owner;
	std::vector<bucket> _buckets;

protected:
	accesskey	_key;
	http::http	_http;
};

} // namespace service

} // namespace alioss

