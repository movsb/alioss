#include <string>
#include <vector>
#include <sstream>

#include <tinyxml2/tinyxml2.h>

#include "signature.h"
#include "misc/time.h"
#include "object.h"
#include "osserror.h"

namespace alioss {
namespace object{

	namespace object_error{


		void no_such_bucket::node_handler(void* n)
		{
			auto node = reinterpret_cast<tinyxml2::XMLElement*>(n);
			if (strcmp(node->Value(), "BucketName") == 0){
				_bucket_name = node->FirstChild()->ToText()->Value();
			}
		}

		void no_such_bucket::dump(std::function<void(const std::string&)> dumper)
		{
			__super::dump(dumper);
			dumper(std::string("BucketName: ") + _bucket_name);
		}


	}

	bool oject::connect()
	{
		return _http.connect(_endpoint.ip().c_str(), _endpoint.port());
	}

	bool oject::disconnect()
	{
		return _http.disconnect();
	}

	bool oject::delete_object(const char* obj)
	{
		if (!obj || !*obj)
			throw ossexcept(ossexcept::kConflict, "Object must have a valid name.", __FUNCTION__);

		connect();

		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		//---------------------------- Requesting----------------------------------
		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "DELETE /" << obj << " HTTP/1.1";
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
		ss << "DELETE\n";
		ss << "\n\n";
		ss << date << "\n";
		ss << "/" << _bkt.name() << "/" << obj; // TODO: url-encoding
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
		0. 200 OK
		1. 204 No Content ---> Success
		2. 404 Not found ---> BucketNotFound
		3. 403 Forbidden
		----------------------------------------------------*/
		auto& status = head.get_status();
		if (status == "200" || status=="204") {
			return true;
		}
		else if (status == "404"){
			auto oe = new object_error::no_such_bucket((char*)bs.data(), bs.size());
			throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}
		else{
			throw ossexcept(ossexcept::ecode(atoi(status.c_str())), head.get_status_n_comment().c_str(), __FUNCTION__);
		}

		return true;
	}



} // namespace object
} // namespace alioss
