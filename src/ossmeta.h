#ifndef __alioss_ossmeta_h__
#define __alioss_ossmeta_h__

#include <string>

namespace alioss {

namespace meta {

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

class bucket{
public:
	bucket() {}

	bucket(const char* name, const char*	loc, const char* date)
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

} // namespace meta

} // namespace alioss

#endif // !__alioss_ossmeta_h__

