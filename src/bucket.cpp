#include <stdexcept>
#include <iostream>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "misc/time.h"
#include "misc/stream.h"
#include "signature.h"
#include "socket.h"
#include "ossmeta.h"
#include "osserror.h"
#include "bucket.h"

namespace alioss{

namespace bucket{

bool bucket::connect()
{
	return _http.connect(_endpoint.ip().c_str(), _endpoint.port());
}

bool bucket::disconnect()
{
	return _http.disconnect();
}

bool bucket::query()
{
	request();
	response();

	return true;
}

bool bucket::request()
{
	auto& head = _http.head();
	head.clear();

	//std::string str;
	std::stringstream ss;

	// Verb
	ss.clear();
	ss.str("");
	ss << "GET /?prefix=windows/&delimiter=/ HTTP/1.1";
	head.set_verb(std::string(ss.str()).c_str());

	// Host
	ss.clear();
	ss.str("");
	ss << _bkt.name() << '.' << _bkt.location() << ".aliyuncs.com";
	head.add_host(std::string(ss.str()).c_str());

	// Date
	std::string date(gmt_time()); // used by here and after
	head.add_date(date.c_str());

	// Authorization
	ss.clear();
	ss.str("");
	ss << "GET\n";
	ss << "\n\n";
	ss << date << "\n";
	ss << "/" << _bkt.name() << "/";
	head.add_authorization(signature(_key, std::string(ss.str())).c_str());

	// Connection
	head.add_connection("close");

	return _http.put_head();
}

bool bucket::response()
{
	_http.get_head();

	std::unique_ptr<http::str_body_ostream> pbs(new http::str_body_ostream);
	_http.get_body(*pbs);
	parse_response_body((char*)pbs->data(), pbs->size());
	
	return true;
}

bool bucket::parse_response_body(const char* data, int size)
{

	std::string s(data, size);
	std::cout << s;

	tinyxml2::XMLDocument xmldoc;
	try{
		if (xmldoc.Parse(data, size_t(size)) != tinyxml2::XMLError::XML_NO_ERROR)
			throw std::runtime_error("[error] " __FUNCTION__ ": xml not well-formed");


	}
	catch(...){
		throw;
	}
	
	return true;
}

bool bucket::delete_()
{
	connect();

	std::stringstream ss;

	auto& head = _http.head();
	head.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
	head.set_verb("DELETE / HTTP/1.1");

	// Host
	ss.clear();ss.str("");
	ss << _bkt.name() << '.' << _bkt.location() << ".aliyuncs.com";
	head.add_host(std::string(ss.str()).c_str());

	//Date
	std::string date(gmt_time());
	head.add_date(date.c_str());

	// Authorization
	ss.clear();ss.str("");
	ss << "DELETE\n";
	ss << "\n\n";
	ss << date << "\n";
	ss << "/" << _bkt.name() << "/";
	head.add_authorization(signature(_key, std::string(ss.str())).c_str());

	// Connection
	head.add_connection("close");

	_http.put_head();

	//-------------------------------------Response--------------------------------
	http::str_body_ostream bs;

	_http.get_head();
	_http.get_body(bs);

	disconnect();

	/*----------------------------------------------------
	ErrorCode:
		1. 204 No Content, Succeeded
		2. 404 Not found, Failed
		3. 409 Conflict, Not empty
		4. 403 Forbidden
	----------------------------------------------------*/
	auto& status = head.get_status();
	if (status == "204"){
		return true;
	}
	else if (status == "404"){
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kNotFound, "Not Found", __FUNCTION__, oe);
	}
	else if (status == "409"){
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kConflict, "Conflict", __FUNCTION__, oe);
	}
	else if (status == "403"){
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kForbidden, "Forbidden", __FUNCTION__, oe);
	}
	else{
		throw "fatal: unknown error occurred!";
	}

	return true;
}


} // namespace bucket

} // namespace alioss

