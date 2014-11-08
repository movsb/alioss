#include <stdexcept>
#include <iostream>

#include "time.h"
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

} // namespace service

} // namespace alioss

