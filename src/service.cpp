#include <iostream>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "misc/time.h"
#include "signature.h"
#include "socket.h"
#include "misc/stream.h"
#include "service.h"
#include "osserror.h"

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

bool service::list_buckets()
{
	_buckets.clear();

	connect();

	auto& head = _http.head();
	head.clear();

	std::stringstream ss;

	//---------------------------- Requesting----------------------------------
	// Verb
	ss.clear(); ss.str("");
	ss << "GET / HTTP/1.1";
	head.set_verb(std::string(ss.str()).c_str());

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

	head.add_authorization(signature(_key, sigstr).c_str());

	// Connection
	head.add_connection("close");

	_http.put_head();

	//-------------------------------------Response--------------------------------
	_http.get_head();

	http::str_body_ostream bs;
	_http.get_body(bs);

	disconnect();

	auto& status = head.get_status();
	if (status == "200") {
		tinyxml2::XMLDocument xmldoc;
		if (xmldoc.Parse((char*)bs.data(), bs.size()) != tinyxml2::XMLError::XML_NO_ERROR)
			throw ossexcept(ossexcept::kXmlError, "Xml not well-formed", __FUNCTION__);

		auto buckets_result = xmldoc.FirstChildElement("ListAllMyBucketsResult");

		tinyxml2::XMLHandle handle_owner(buckets_result->FirstChildElement("Owner"));
		_owner.set_id(handle_owner.FirstChildElement("ID").FirstChild().ToText()->Value());
		_owner.set_display_name(handle_owner.FirstChildElement("DisplayName").FirstChild().ToText()->Value());

		auto node_buckets = buckets_result->FirstChildElement("Buckets");
		if(node_buckets != nullptr){
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

		return true;
	}
	else{
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
	}
}

bool service::verify_user()
{
	connect();

	auto& head = _http.head();
	head.clear();

	std::stringstream ss;

	//---------------------------- Requesting----------------------------------
	// Verb
	ss.clear(); ss.str("");
	// TODO: max-keys seems not working?
	ss << "GET /?max-keys=0 HTTP/1.1"; // max-keys set, different from list_buckets()
	head.set_verb(std::string(ss.str()).c_str());

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

	head.add_authorization(signature(_key, sigstr).c_str());

	// Connection
	head.add_connection("close");

	_http.put_head();

	//-------------------------------------Response--------------------------------
	_http.get_head();

	http::str_body_ostream bs;
	_http.get_body(bs);

	disconnect();

	auto& status = head.get_status();
	if (status == "200") {
		return true;
	}
	else{
		auto oe = new osserr(bs.data(), bs.size());
		throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
	}
}

void service::dump_buckets(std::function<void(int i, const meta::bucket& bkt)> dumper)
{
	int i = 0;
	for (auto& b : _buckets){
		dumper(++i, b);
	}
}



} // namespace service

} // namespace alioss

