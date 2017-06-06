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
		osserr::dump(dumper);

		dumper(std::string("LocationConstraint: ") + _location);
	}

	void invalid_bucket_name::node_handler(void* n)
	{
		auto node = reinterpret_cast<tinyxml2::XMLElement*>(n);
		if (strcmp(node->Value(), "BucketName") == 0){
            auto first = node->FirstChild();
            if(first && first->ToText())
                _bucket_name = first->ToText()->Value();
            else
                _bucket_name = "";
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

bool bucket::list_objects(const char* folder, bool recursive)
{
	std::stringstream ss;

	auto& head = _http.head();
	head.clear();

	_contents.clear();
	_common_prefixes.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
	ss.clear(); ss.str("");
	ss << "GET /?prefix=" << (folder && *folder=='/' ? folder+1 : "")
		<< "&delimiter=" << (recursive ? "" : "%2F")
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

	connect();

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
		if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_SUCCESS){
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
	if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_SUCCESS){
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

