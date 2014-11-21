#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstring>
#include <regex>

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
		osserr::dump(dumper);

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
		osserr::dump(dumper);

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

bool bucket::delete_bucket()
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
	if (status == "204") {
		return true;
	}
	else{
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
	}
	
	return true;
}

bool bucket::create_bucket()
{
	if (!std::regex_match(_bkt.name(), std::regex(R"(^[a-z0-9]{1}[a-z0-9\-]{1,61}[a-z0-9]{1}$)")))
		throw ossexcept(ossexcept::kInvalidArgs, "bucket name is invalid", __FUNCTION__);

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
	4. 400, TooManyBuckets
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
		else if (ec == "TooManyBuckets"){
			auto oe = new osserr(&doc);
			throw ossexcept(ossexcept::kTooMany, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}

		throw ossexcept(ossexcept::kUnhandled, "fatal: unhandled exception!", __FUNCTION__);
	}
	else{
		throw ossexcept(ossexcept::kXmlError, head.get_status().c_str(), __FUNCTION__);
	}

	return true;
}

bool bucket::list_objects(const char* folder, bool recursive)
{
	connect();

	std::stringstream ss;

	auto& head = _http.head();
	head.clear();

	_contents.clear();
	_common_prefixes.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
	ss.clear(); ss.str("");
	ss << "GET /?prefix=" << folder
		<< "&delimiter=" << (recursive ? "" : "/")
		<< " HTTP/1.1";
	head.set_verb(std::string(ss.str()).c_str());

	// Host
	ss.clear(); ss.str("");
	ss << _bkt.name() << '.' << _bkt.location() << ".aliyuncs.com";
	head.add_host(std::string(ss.str()).c_str());

	//Date
	std::string date(gmt_time());
	head.add_date(date.c_str());

	// Authorization
	ss.clear(); ss.str("");
	ss << "GET\n";
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
	1. 204 OK
	2. 404 Not found, NoSuchBucket
	3. 400 Bad Request
	4. 403 Forbidden
	----------------------------------------------------*/
	auto& status = head.get_status();
	if (status == "200") {
		tinyxml2::XMLDocument doc;
		if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_NO_ERROR){
			// TODO: UTF-8 convert (Windows)
			auto List_bucket_result = doc.FirstChildElement("ListBucketResult");
			
			//TODO: isTruncated

			// Travels All Contents. why name an Object `Contents'? why ain't its container?
			auto la_get_value = [](tinyxml2::XMLElement* e, const char* name)->const char*{
				auto node = e->FirstChildElement(name);
				return node->FirstChild()->ToText()->Value();
			};

			for (auto onec = List_bucket_result->FirstChildElement("Contents");
				onec != nullptr;
				onec = onec->NextSiblingElement("Contents"))
			{
				auto& cont = create_content();
				cont.set_key(la_get_value(onec, "Key"));
				cont.set_last_modified(la_get_value(onec, "LastModified"));
				cont.set_e_tag(la_get_value(onec, "ETag"));
				cont.set_type(la_get_value(onec, "Type"));
				cont.set_size(la_get_value(onec, "Size"));
				cont.set_storage_class(la_get_value(onec, "StorageClass"));
				cont.set_owner( // so long?
					onec->FirstChildElement("Owner")->FirstChild()->FirstChild()->ToText()->Value(),
					onec->FirstChildElement("Owner")->FirstChild()->NextSibling()->FirstChild()->ToText()->Value()
					);
			}

			// Travels all directories. Its xml style is hard to understand.
			for (auto oned = List_bucket_result->FirstChildElement("CommonPrefixes");
				oned != nullptr;
				oned = oned->NextSiblingElement("CommonPrefixes"))
			{
				auto& dir = create_common_prefix();
				dir = oned->FirstChild()->FirstChild()->ToText()->Value();
			}
		}
		else
			throw ossexcept(ossexcept::kXmlError, "fatal: xml not well-formed", __FUNCTION__);
		return true;
	}

	tinyxml2::XMLDocument doc;
	if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_NO_ERROR){
		std::string ec(doc.FirstChildElement("Error")->FirstChildElement("Code")->FirstChild()->ToText()->Value());
		if (ec == "NoSuchBucket" || ec == "AccessDenied" || ec == "InvalidArgument"){
			auto oe = new bucket_error::invalid_location_constraint(&doc);
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}
		throw ossexcept(ossexcept::kUnhandled, "fatal: unhandled exception!", __FUNCTION__);
	}
	else{
		throw ossexcept(ossexcept::kXmlError,"fatal: xml not well-formed", __FUNCTION__);
	}

	return true;
}

void bucket::dump_objects(std::function<bool(int i, const meta::content& object)> dumper)
{
	int i = 0;
	for (auto& obj : _contents){
		dumper(++i, obj);
	}
}

void bucket::dump_folders(std::function<bool(int i, const std::string& folder)> dumper)
{
	int i = 0;
	// What's `fir'? I DON'T know why I give a name like that.
	for (auto& fir : _common_prefixes){
		dumper(++i, fir);
	}
}

} // namespace bucket

} // namespace alioss

