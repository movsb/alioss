#include <stdexcept>
#include <iostream>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "misc/time.h"
#include "signature.h"
#include "socket.h"
#include "misc/stream.h"
#include "service.h"

namespace alioss{

namespace service{

bool service::connect()
{
	socket::resolver resolver;
	resolver.resolve("oss.aliyuncs.com", "http");
	return _http.connect(resolver[0].c_str(), 80);
}

bool service::disconnect()
{
	return _http.disconnect();
}

bool service::query()
{
	request();
	response();

	return true;
}

bool service::request()
{
	auto& head = _http.head();
	head.clear();

	std::string str;
	std::stringstream ss;

	// Verb
	ss.clear();
	ss << "GET / HTTP/1.1";
	str = ss.str();
	head.set_verb(str.c_str());

	// Host
	head.add_host("oss.aliyuncs.com");

	// Date
	std::string date(gmt_time());
	head.add_date(date.c_str());

	// Authorization
	std::string sigstr;
	sigstr += "GET\n";		// Verb
	sigstr += "\n\n";		// Content-MD5 & Content-Type
	sigstr += date + "\n";	// GMT Date/Time
	sigstr += "/";			// Resource

	std::string authstr(signature(_key, sigstr));
	head.add_authorization(authstr.c_str());

	// Connection
	head.add_connection("close");

	return _http.put_head();
}

bool service::response()
{
	_http.get_head();

	std::unique_ptr<body_stream> pbs(new body_stream);
	_http.get_body(*pbs);
	parse_response_body((char*)pbs->data(), pbs->size());
	
	return true;
}

bool service::parse_response_body(const char* data, int size)
{
	tinyxml2::XMLDocument xmldoc;
	try{
		if (xmldoc.Parse(data, size_t(size)) != tinyxml2::XMLError::XML_NO_ERROR)
			throw std::runtime_error("[error] " __FUNCTION__ ": xml not well-formed");

		auto buckets_result = xmldoc.FirstChildElement("ListAllMyBucketsResult");
		if (buckets_result == nullptr)
			throw std::runtime_error("[error] " __FUNCTION__ ": node 'ListAllMyBucketsResult' not been found");

		tinyxml2::XMLHandle handle_owner(buckets_result->FirstChildElement("Owner"));
		_owner.set_id(handle_owner.FirstChildElement("ID").FirstChild().ToText()->Value());
		_owner.set_display_name(handle_owner.FirstChildElement("DisplayName").FirstChild().ToText()->Value());

		auto node_buckets = buckets_result->FirstChildElement("Buckets");
		for (auto node = node_buckets->FirstChildElement("Bucket");
			node != nullptr;
			node = node->NextSiblingElement("Bucket"))
		{
			tinyxml2::XMLHandle handle_bucket(node);
			auto& newbkt = bucket_create();
			newbkt.set_name(handle_bucket.FirstChildElement("Name").FirstChild().ToText()->Value());
			newbkt.set_location(handle_bucket.FirstChildElement("Location").FirstChild().ToText()->Value());
			newbkt.set_creation_date(handle_bucket.FirstChildElement("CreationDate").FirstChild().ToText()->Value());
		}

	}
	catch(...){
		throw;
	}
	
	return true;
}


} // namespace service

} // namespace alioss

