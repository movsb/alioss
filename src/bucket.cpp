#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstring>

#include <tinyxml2/tinyxml2.h>

#include "crypto/crypto.h"
#include "misc/time.h"
#include "misc/stream.h"
#include "misc/strutil.h"

#include "sign.h"
#include "socket.h"
#include "ossmeta.h"
#include "osserror.h"
#include "bucket.h"

namespace alioss{

namespace bucket{

bool bucket::connect()
{
	return _http.connect(_endpoint.ip().c_str(), _endpoint.port());
}

bool bucket::disconnect()
{
	return _http.disconnect();
}

void bucket::_list_objects_internal(const std::string& prefix, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders)
{
	auto& head = _http.head();
	head.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
    head.set_request("GET", "/", 
        {
            {"prefix", prefix},
            {"delimiter", recursive ? "" : "/"},
            { "max-keys", "1000" },
        }
    );

	// Host
	head.add_host(_domain);

	//Date
	std::string date(gmt_time());
	head.add_date(date.c_str());

	// Authorization
	head.add_authorization(sign_head(_key, "GET", date, '/' + _name + '/'));

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

            auto& objs = *objects;
            auto& dirs = *folders;

            objs.clear();
            dirs.clear();

            auto la_is_folder = [](const std::string& key) {
                return !key.empty() && key.back() == '/';
            };

			for (auto onec = List_bucket_result->FirstChildElement("Contents");
				onec != nullptr;
				onec = onec->NextSiblingElement("Contents"))
			{
                auto key = (std::string)la_get_value(onec, "Key");
                if (!la_is_folder(key)) {
                    meta::content file;

                    file.set_key(('/' + key).c_str());
                    file.set_last_modified(la_get_value(onec, "LastModified"));
                    file.set_e_tag(la_get_value(onec, "ETag"));
                    file.set_type(la_get_value(onec, "Type"));
                    file.set_size(la_get_value(onec, "Size"));
                    file.set_storage_class(la_get_value(onec, "StorageClass"));
                    file.set_owner( // so long?
                        onec->FirstChildElement("Owner")->FirstChild()->FirstChild()->ToText()->Value(),
                        onec->FirstChildElement("Owner")->FirstChild()->NextSibling()->FirstChild()->ToText()->Value()
                        );

                    objs.push_back(std::move(file));
                }
                else {
                    dirs.push_back('/' + key);
                }
			}

			// Travels all directories. Its xml style is hard to understand.
			for (auto oned = List_bucket_result->FirstChildElement("CommonPrefixes");
				oned != nullptr;
				oned = oned->NextSiblingElement("CommonPrefixes"))
			{
				auto dir = oned->FirstChild()->FirstChild()->ToText()->Value();
                dirs.push_back('/' + dir);
			}

            return;
		}
        else {
			throw ossexcept(ossexcept::kXmlError, "fatal: xml not well-formed", __FUNCTION__);
        }
	}

	tinyxml2::XMLDocument doc;
	if (doc.Parse((char*)bs.data(), bs.size()) == tinyxml2::XMLError::XML_SUCCESS){
		std::string ec(doc.FirstChildElement("Error")->FirstChildElement("Code")->FirstChild()->ToText()->Value());
		if (ec == "NoSuchBucket" || ec == "AccessDenied" || ec == "InvalidArgument"){
			throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__, NULL);
		}
		throw ossexcept(ossexcept::kUnhandled, "fatal: unhandled exception!", __FUNCTION__);
	}
	else{
		throw ossexcept(ossexcept::kXmlError,"fatal: xml not well-formed", __FUNCTION__);
	}
}


void bucket::list_objects(const std::string& prefix, std::vector<meta::content>* objects, std::vector<std::string>* folders)
{
    if (prefix.empty() || prefix[0] != '/') {
        throw ossexcept(ossexcept::kInvalidPath);
    }

    return _list_objects_internal(prefix.substr(1), true, objects, folders);
}


void bucket::list_objects(const std::string& folder, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders)
{
    if (folder.empty() || folder[0] != '/') {
        throw ossexcept(ossexcept::kInvalidPath);
    }

    auto prefix = folder.substr(1);

    if (!prefix.empty() && prefix.back() != '/') {
        prefix += '/';
    }

    return _list_objects_internal(prefix, recursive, objects, folders);
}

bool bucket::delete_object(const std::string& obj)
{
    auto& head = _http.head();
    head.clear();

    //---------------------------- Requesting----------------------------------
    // Verb
    head.set_request("DELETE", obj);

    // Host
    head.add_host(_domain);

    //Date
    std::string date(gmt_time());
    head.add_date(date.c_str());

    // Authorization
    head.add_authorization(sign_head(_key, "DELETE", date, '/' + _name + obj));

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
        // TODO
        // auto oe = new bucket_error::no_such_bucket((char*)bs.data(), bs.size());
        // throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
    }
    else{
        throw ossexcept(ossexcept::ecode(atoi(status.c_str())), head.get_status_n_comment().c_str(), __FUNCTION__);
    }

    return true;
}

bool bucket::get_object(const std::string& obj, stream::ostream& os, http::getter getter, const std::string& range/*=""*/, const std::string& unmodified_since/*=""*/)
{
    //---------------------------- Requesting----------------------------------
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("GET", obj);

    // Host
    head.add_host(_domain);

    //Date
    std::string date(gmt_time());
    head.add_date(date.c_str());

    // Authorization
    head.add_authorization(sign_head(_key, "GET", date, '/' + _name + obj));

    // Range & Unmodified-Since
    if (range.size()){
        head.add("Range", "bytes=" + range);
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
        _http.get_body(os, getter);
        disconnect();
        return true;
    }
    else if (status == "412"){
        disconnect();
        throw ossexcept(ossexcept::kNotSpecified, head.get_status_n_comment().c_str(), __FUNCTION__);
    }
    else if (status == "404"){
        http::str_body_ostream bs;
        _http.get_body(bs);
        disconnect();

        auto oe = new osserr((char*)bs.data(), bs.size());
        throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
    }
    else if(status == "403") {
        http::str_body_ostream bs;
        _http.get_body(bs);
        auto oe = new osserr((char*)bs.data(), bs.size());
        throw ossexcept(ossexcept::kForbidden, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
    }
    else{
        disconnect();
        throw ossexcept(ossexcept::kUnhandled, head.get_status_n_comment().c_str(), __FUNCTION__);
    }
}

bool bucket::put_object(const std::string& obj, stream::istream& is, http::putter putter,
    const char* content_type/*=""*/, const char* content_disposition/*=""*/, 
    const char* content_encoding/*=""*/)
{
    //---------------------------- Requesting----------------------------------
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("PUT", obj);

    // Content-Length & Content-Type && Content-Disposition && Content-Encoding
    head.add_content_length(is.size());
    if (!content_type || !*content_type) content_type = "text/json";
    head.add_content_type(content_type);
    if (content_disposition && *content_disposition)
        head.add_content_disposition(content_disposition);
    if (content_encoding && *content_encoding)
        head.add_content_encoding(content_encoding);

    // Host
    head.add_host(_domain);

    //Date
    std::string date(gmt_time());
    head.add_date(date.c_str());

    // Authorization
    head.add_authorization(sign_head(_key, "PUT", date, '/' + _name + obj));

    // Connection
    head.add_connection("close");

    connect();

    _http.put_head();

    crypto::cmd5 body_md5;
    _http.put_body(is, [&](const unsigned char* data, int sz, int total, int transferred){
        body_md5.update(data, sz);
        if(putter) putter(data, sz, total, transferred);
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

bool bucket::create_folder(const std::string& name)
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

const http::header::head& bucket::head_object(
    const std::string& obj,
    const char* if_modified_since /*= nullptr*/,
    const char* if_unmodified_since /*= nullptr*/,
    const char* if_match /*= nullptr*/,
    const char* if_none_match /*= nullptr */)
{
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("HEAD", obj);

    // Host
    head.add_host(_domain);

    //Date
    std::string date(gmt_time());
    head.add_date(date);

    // Authorization
    head.add_authorization(sign_head(_key, "HEAD", date, '/' + _name + obj));

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
} // namespace bucket

} // namespace alioss

