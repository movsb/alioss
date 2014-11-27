#include <iostream>
#include <memory>

#include <tinyxml2/tinyxml2.h>

#include "misc/strutil.h"
#include "misc/time.h"
#include "signature.h"
#include "socket.h"
#include "misc/stream.h"
#include "service.h"
#include "osserror.h"
#include "ossmeta.h"

namespace alioss{

namespace service{

bool service::list_buckets()
{
	using namespace strutil;

	auto& head = _http.head();
	head.clear();

	std::stringstream ss;

	// Verb
	clear(ss);
	ss << "GET / HTTP/1.1";
	head.set_verb(std::string(ss.str()));

	// Host
	head.add_host(meta::oss_root_server);

	// Date
	std::string date(gmt_time());
	head.add_date(date);

	// Authorization
	std::string sigstr("GET\n\n\n");
	sigstr += date + "\n/";
	head.add_authorization(signature(_key, sigstr));

	// Connection
	head.add_connection("close");

	_http.connect(_endpoint);
	_http.put_head();

	_http.get_head();

	http::str_body_ostream bs;
	_http.get_body(bs);

	_http.disconnect();

	_buckets.clear();

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

void service::dump_buckets(std::function<void(int i, const meta::bucket& bkt)> dumper)
{
	int i = 0;
	for (auto& b : _buckets){
		dumper(++i, b);
	}
}

bool service::verify_user()
{
	try{
		list_buckets();
		return true;
	}
	catch (ossexcept& e){
		e.push_stack(__FUNCTION__);
		throw;
	}
}

} // namespace service

} // namespace alioss

