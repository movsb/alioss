#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <cstring>
#include <functional>

#include <tinyxml2/tinyxml2.h>

#include "signature.h"
#include "misc/time.h"
#include "object.h"
#include "osserror.h"
#include "crypto/crypto.h"

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
			osserr::dump(dumper);
			dumper(std::string("BucketName: ") + _bucket_name);
		}


	}

	bool object::connect()
	{
		return _http.connect(_endpoint.ip().c_str(), _endpoint.port());
	}

	bool object::disconnect()
	{
		return _http.disconnect();
	}

	bool object::delete_object(const char* obj)
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

	bool object::get_object(const char* obj, stream::ostream& os, const std::string& range/*=""*/, const std::string& unmodified_since/*=""*/)
	{
		//----------------------------------Argument Checking-------------------------------------
		if (!obj || !* obj || !is_object_name_valid(obj)) throw ossexcept(ossexcept::kInvalidArgs, "Invalid argument: obj", __FUNCTION__);

		if (range.size()){
			std::regex re_range(R"(\d-\d)");
			if (!std::regex_match(range, re_range))
				throw ossexcept(ossexcept::kInvalidArgs, "Invalid argument: range", __FUNCTION__);
			else{
				int s, e;
				if (sscanf(range.c_str(), "%d-%d", &s, &e) != 2
					|| s < 0 || e < 0
					|| s > e)
				{
					throw ossexcept(ossexcept::kInvalidArgs, "Invalid argument: range", __FUNCTION__);
				}
			}


			std::regex re_gmt(R"([A-Z]{1}[a-z]{2}, [0-9]{2} [A-Z]{1}[a-z]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} GMT)");
			if (!std::regex_match(unmodified_since, re_gmt))
				throw ossexcept(ossexcept::kInvalidArgs, "Invalid argument: unmodified_since", __FUNCTION__);

		}

		//---------------------------- Requesting----------------------------------
		connect();

		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "GET /" << obj << " HTTP/1.1";
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
		ss << "/" << _bkt.name() << "/" << obj; // TODO: url-encoding
		head.add_authorization(signature(_key, std::string(ss.str())).c_str());

		// Range & Unmodified-Since
		if (range.size()){
			ss.clear(); ss.str("");
			ss << "bytes=" << range;
			head.add("Range", std::string(ss.str()).c_str());

			head.add("If-Unmodified-Since", unmodified_since.c_str());
		}

		// Connection
		head.add_connection("close");

		_http.put_head();

		//-------------------------------------Response--------------------------------
		_http.get_head();
		
		auto& status = head.get_status();
		if (status == "200"){
			_http.get_body(os);
			disconnect();
			return true;
		}
		else if (status == "412"){
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__);
			disconnect();
		}
		else if (status == "404"){
			http::str_body_ostream bs;
			_http.get_body(bs);
			disconnect();

			auto oe = new osserr((char*)bs.data(), bs.size());
			throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
		}
		else{
			disconnect();
			throw ossexcept(ossexcept::kUnhandled, head.get_status_n_comment().c_str(), __FUNCTION__);
		}
	}

	// not implemented
	bool object::is_object_name_valid(const char* obj)
	{
		return true;
	}

	bool object::put_object(const char* obj, stream::istream& is, 
		const char* content_type/*=""*/, const char* content_disposition/*=""*/, 
		const char* content_encoding/*=""*/)
	{
		//----------------------------------Argument Checking-------------------------------------
		if (!obj || !* obj || !is_object_name_valid(obj)) throw ossexcept(ossexcept::kInvalidArgs, "Invalid argument: obj", __FUNCTION__);

		//---------------------------- Requesting----------------------------------
		connect();

		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "PUT /" << obj << " HTTP/1.1";
		head.set_verb(std::string(ss.str()).c_str());

		// Content-Length & Content-Type && Content-Disposition && Content-Encoding
		head.add_content_length(is.size());
		if (!content_type || !*content_type) content_type = "text/json";
		head.add_content_type(content_type);
		if (content_disposition && *content_disposition)
			head.add_content_disposition(content_disposition);
		if (content_encoding && *content_encoding)
			head.add_content_encoding(content_encoding);

		// Host
		ss.clear(); ss.str("");
		ss << _bkt.name() << '.' << _bkt.location() << ".aliyuncs.com";
		head.add_host(std::string(ss.str()).c_str());

		//Date
		std::string date(gmt_time());
		head.add_date(date.c_str());

		// Authorization
		ss.clear(); ss.str("");
		ss << "PUT\n";
		ss << "\n";
		ss << content_type << "\n";
		ss << date << "\n";
		ss << "/" << _bkt.name() << "/" << obj; // TODO: url-encoding
		head.add_authorization(signature(_key, std::string(ss.str())).c_str());

		// Connection
		head.add_connection("close");

		_http.put_head();

		crypto::cmd5 body_md5;
		_http.put_body(is, [&](const unsigned char* data, int sz){
			body_md5.update(data, sz);
		});
		body_md5.finish();

		//-------------------------------------Response--------------------------------
		http::str_body_ostream bs;
		_http.get_head();
		_http.get_body(bs);
		disconnect();

		auto& status = head.get_status();
		if (status == "200"){
			std::string etag;
			if (head.get("ETag", &etag) && etag.size()>2){
				if (body_md5.str() != std::string(etag.c_str() + 1, 32)){
					throw ossexcept(ossexcept::kInvalidCheckSum, etag.c_str(), __FUNCTION__);
				}
			}
			return true;
		}

		else{
			throw ossexcept(ossexcept::kUnhandled, head.get_status_n_comment().c_str(), __FUNCTION__);
		}
	}

	bool object::create_folder(const char* name)
	{
		class empty_istream : public stream::istream
		{
		public:
			virtual int size() const{
				return 0;
			}
			virtual int read_some(unsigned char* buf, int sz){
				return 0;
			}
		};

		try{
			empty_istream is;
			put_object(name, is);
			return true;
		}
		catch (ossexcept& e){
			e.push_stack(__FUNCTION__);
			throw;
		}

		return true;
	}






} // namespace object
} // namespace alioss
