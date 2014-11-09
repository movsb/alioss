#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstring>

#include <tinyxml2/tinyxml2.h>

#include "crypto/crypto.h"
#include "misc/time.h"
#include "misc/stream.h"
#include "signature.h"
#include "socket.h"
#include "ossmeta.h"
#include "osserror.h"
#include "bucket.h"

namespace alioss{

namespace bucket{

namespace bucket_error{

	void invalid_location_constraint::node_handler(void* n)
	{
		auto node = reinterpret_cast<tinyxml2::XMLElement*>(n);
		if (strcmp(node->Value(), "LocationConstraint") == 0){
			_location = node->FirstChild()->ToText()->Value();
		}
	}

	void invalid_location_constraint::dump(std::function<void(const std::string&)> dumper)
	{
		__super::dump(dumper);

		dumper(std::string("LocationConstraint: ") + _location);
	}

	void invalid_bucket_name::node_handler(void* n)
	{
		auto node = reinterpret_cast<tinyxml2::XMLElement*>(n);
		if (strcmp(node->Value(), "BucketName") == 0){
			_bucket_name = node->FirstChild()->ToText()->Value();
		}
	}

	void invalid_bucket_name::dump(std::function<void(const std::string&)> dumper)
	{
		__super::dump(dumper);

		dumper(std::string("BucketName: ") + _bucket_name);
	}


}

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

bool bucket::create_bucket()
{
	connect();

	std::stringstream ss;

	auto& head = _http.head();
	head.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
	head.set_verb("PUT / HTTP/1.1");

	// Host
	std::string location = _bkt.location().size() ? _bkt.location() : "oss-cn-hangzhou";
	head.add_host((_bkt.name() + '.' + location + ".aliyuncs.com").c_str());

	//Date
	std::string date(gmt_time());
	head.add_date(date.c_str());

	// body
	std::string body(R"(<?xml version="1.0" encoding="UTF-8"?><CreateBucketConfiguration><LocationConstraint>)");
	body += location;
	body += R"(</LocationConstraint></CreateBucketConfiguration>)";

	// Content-*
	head.add(http::header::kContentType, "text/xml");

	ss.clear(); ss.str(""); ss << body.size();
	head.add(http::header::kContentLength, std::string(ss.str()).c_str());

	// Content-MD5
	// auto md5 = content_md5(body.c_str(), body.size());

	// Authorization
	ss.clear(); ss.str("");
	ss << "PUT\n"
		<< "\n" // no content-md5 requested
		<< "text/xml\n"
		<< date << "\n"
		<< '/' << _bkt.name() << '/';

	head.add_authorization(signature(_key, std::string(ss.str())).c_str());

	// Connection
	head.add_connection("close");

	_http.put_head();
	_http.put_body(body.c_str(), body.size());

	//-------------------------------------Response--------------------------------
	http::str_body_ostream bs;

	_http.get_head();
	_http.get_body(bs);

	disconnect();

	/*----------------------------------------------------
	ErrorCode:
	1. 200, OK
	2. 400, InvalidLocationConstraint
	3. 400, InvalidBucketName
	----------------------------------------------------*/
	auto& status = head.get_status();
	if (status == "200") return true;

	tinyxml2::XMLDocument doc;
	if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_NO_ERROR){
		std::string ec(doc.FirstChildElement("Error")->FirstChildElement("Code")->FirstChild()->ToText()->Value());
		if (ec == "InvalidLocationConstraint"){
			auto oe = new bucket_error::invalid_location_constraint(&doc);
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}
		else if (ec == "InvalidBucketName"){
			auto oe = new bucket_error::invalid_bucket_name(&doc);
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}
		else if (ec == "BucketAlreadyExists"){
			auto oe = new osserr(&doc);
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}

		throw ossexcept(ossexcept::kUnhandled, "fatal: unhandled exception!", __FUNCTION__);
	}
	else{
		throw ossexcept(ossexcept::kXmlError, head.get_status().c_str(), __FUNCTION__);
	}

	return true;
}



} // namespace bucket

} // namespace alioss

