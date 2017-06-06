#ifndef __alioss_ossmeta_h__
#define __alioss_ossmeta_h__

#include <string>

namespace alioss {

namespace meta {

    const char* const oss_root_server = "oss.ayuncs.com";
    const char* const oss_server_suffix = ".aliyuncs.com";

	struct oss_data_center_s{
		const char* name;
		const char* location;
	};

	const int oss_data_center_count = 8;
	extern oss_data_center_s oss_data_center[];

class owner {
public:
	owner() {}
	owner(const std::string& id, const std::string& name)
		: _id(id)
		, _display_name(name)
	{}

	void set_id(const std::string& id) {
		_id = id;
	}

	void set_display_name(const std::string& name){
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

class bucket{
public:
	bucket() {}

	bucket(const std::string& name, const std::string& loc, const std::string& date)
		: _name(name)
		, _location(loc)
		, _creation_date(date)
	{}

	void set_name(const std::string& name) {
		_name = name;
	}
	const std::string& name() const {
		return _name;
	}

	void set_location(const std::string& loc){
		_location = loc;
	}
	const std::string& location() const {
		return _location;
	}

	void set_creation_date(const std::string& date){
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

class content {
public:
	/// setters
	void set_key(const char* v)				{ _key = v; }
	void set_last_modified(const char* v)	{ _last_modified = v; }
	void set_e_tag(const char* v)			{ _e_tag = v; }
	void set_type(const char* v)			{ _type = v; }
	void set_size(const char* v)			{ _size = v; }
	void set_storage_class(const char* v)	{ _storage_class = v; }

	/// getters
	const std::string& key() const			{ return _key; }
	const std::string& last_modified() const{ return _last_modified; }
	const std::string& e_tag() const		{ return _e_tag; }
	const std::string& type() const 		{ return _type; }
	const std::string& size() const			{ return _size; }
	const std::string& storage_class() const{ return _storage_class; }

	/// others
	void set_owner(const char* id, const char* name) {
		_owner.set_id(id);
		_owner.set_display_name(name);
	}

	// here I want to use the name `owner',
	// but it seems to be ambiguous. So I add
	// a namespace `meta' as prefix. It works!
	meta::owner& owner() { return _owner; }

protected:
	std::string		_key;
	std::string		_last_modified;
	std::string		_e_tag;
	std::string		_type;
	std::string		_size;
	std::string		_storage_class;
	
	meta::owner			_owner;

};

} // namespace meta

} // namespace alioss

#endif // !__alioss_ossmeta_h__

