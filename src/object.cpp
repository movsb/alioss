#include <string>
#include <vector>
#include <sstream>
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
		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		//---------------------------- Requesting----------------------------------
		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "DELETE " << obj << " HTTP/1.1";
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
		ss << "/" << _bkt.name() << obj; // TODO: url-encoding
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
		//---------------------------- Requesting----------------------------------
		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "GET " << obj << " HTTP/1.1";
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
		ss << "/" << _bkt.name() << obj; // TODO: url-encoding
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

        connect();

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

	bool object::put_object(const char* obj, stream::istream& is, 
		const char* content_type/*=""*/, const char* content_disposition/*=""*/, 
		const char* content_encoding/*=""*/)
	{
		//---------------------------- Requesting----------------------------------
		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "PUT " << obj << " HTTP/1.1";
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
		ss << "/" << _bkt.name() << obj; // TODO: url-encoding
		head.add_authorization(signature(_key, std::string(ss.str())).c_str());

		// Connection
		head.add_connection("close");

        connect();

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

	const http::header::head& object::head_object(
		const char* obj,
		const char* if_modified_since /*= nullptr*/,
		const char* if_unmodified_since /*= nullptr*/,
		const char* if_match /*= nullptr*/,
		const char* if_none_match /*= nullptr */)
	{
		std::stringstream ss;

		auto& head = _http.head();
		head.clear();

		// Verb
		ss.clear(); ss.str("");
		// TODO: url-encoding
		ss << "HEAD " << obj << " HTTP/1.1";
		head.set_verb(std::string(ss.str()));

		// Host
		ss.clear(); ss.str("");
		ss << _bkt.name() << '.' << _bkt.location() << ".aliyuncs.com";
		head.add_host(std::string(ss.str()));

		//Date
		std::string date(gmt_time());
		head.add_date(date);

		// Authorization
		ss.clear(); ss.str("");
		ss << "HEAD\n";
		ss << "\n\n";
		ss << date << "\n";
		ss << "/" << _bkt.name() << obj; // TODO: url-encoding
		head.add_authorization(signature(_key, std::string(ss.str())));

		// Connection
		head.add_connection("close");

		// Queries.
		struct{ const char* h; http::header::FIELD f; } a[] = {
				{ if_modified_since, http::header::kIfModifiedSince },
				{ if_unmodified_since, http::header::kIfUnmodifiedSince },
				{ if_match, http::header::kIfMatch },
				{ if_none_match, http::header::kIfNoneMatch },
		};
		for (auto& oa : a){
			if (oa.h && *oa.h)
				head.add(oa.f, oa.h);
		}

		try{
			connect();
			_http.put_head();
			_http.get_head();
			disconnect();
		}
		catch (socket::socketexcept& e){
			e.push_stack(__FUNCTION__);
			throw;
		}

		if (head.get_status() == "200")
			return _http.head();
		else if (head.get_status() == "304")
			throw ossexcept(ossexcept::kNotModified, head.get_status_n_comment().c_str(), __FUNCTION__);
		else if (head.get_status() == "404")
			throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__);
		else if (head.get_status() == "412")
			throw ossexcept(ossexcept::kPreconditionFailed, head.get_status_n_comment().c_str(), __FUNCTION__);
		else
			throw ossexcept(ossexcept::kUnhandled, head.get_status_n_comment().c_str(), __FUNCTION__);
	}
		
} // namespace object
} // namespace alioss
