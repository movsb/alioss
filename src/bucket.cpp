#include <stdexcept>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <cstring>

#include <tinyxml2/tinyxml2.h>

#include "socket.h" // put here to fix winsock2 include problem

#include "crypto/crypto.h"
#include "misc/time.h"
#include "misc/stream.h"
#include "misc/strutil.h"
#include "misc/file_system.h"

#include "sign.h"
#include "ossmeta.h"
#include "osserror.h"
#include "bucket.h"

namespace alioss{

namespace bucket{

void bucket::set_endpoint(const std::string& bucket_name, const std::string& bucket_location)
{
    _name = bucket_name;
    _location = bucket_location;
    _host = meta::make_public_host(bucket_name, bucket_location);
    set_endpoint_base::set_endpoint(_host, "http");
}

bool bucket::connect()
{
	return _http.connect(_ep.ip().c_str(), _ep.port());
}

bool bucket::disconnect()
{
	return _http.disconnect();
}

bool bucket::_list_objects_internal(const std::string & prefix, const std::string& marker, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders, std::string* next_marker, std::unordered_set<std::string>* prefixes)
{
	auto& head = _http.head();
	head.clear();

	//---------------------------- Requesting----------------------------------
	// Verb
    head.set_request("GET", "/", 
        {
            { "prefix", prefix},
            { "delimiter", recursive ? "" : "/"},
            { "max-keys", "100" },
            { "marker", marker},
        }
    );

	// Host
	head.add_host(_host);

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
			
            auto is_truncated = List_bucket_result->FirstChildElement("IsTruncated")->FirstChild()->ToText()->Value() == std::string("true");
            if(is_truncated) {
                *next_marker = List_bucket_result->FirstChildElement("NextMarker")->FirstChild()->ToText()->Value();
            }

			auto la_get_value = [](tinyxml2::XMLElement* e, const char* name) {
				auto node = e->FirstChildElement(name);
				return node->FirstChild()->ToText()->Value();
			};

            auto& objs = *objects;
            auto& dirs = *folders;

			for (auto onec = List_bucket_result->FirstChildElement("Contents");
				onec != nullptr;
				onec = onec->NextSiblingElement("Contents"))
			{
                auto key = (std::string)la_get_value(onec, "Key");
                auto is_file = key.back() != '/';

                if (!recursive || is_file) {
                    meta::content file;

                    file.set_key(('/' + key).c_str());
                    file.set_last_modified(la_get_value(onec, "LastModified"));
                    file.set_e_tag(la_get_value(onec, "ETag"));
                    file.set_type(la_get_value(onec, "Type"));
                    file.set_size(la_get_value(onec, "Size"));
                    file.set_storage_class(la_get_value(onec, "StorageClass"));
                    file.set_owner(
                        onec->FirstChildElement("Owner")->FirstChild()->FirstChild()->ToText()->Value(),
                        onec->FirstChildElement("Owner")->FirstChild()->NextSibling()->FirstChild()->ToText()->Value()
                        );

                    objs.push_back(std::move(file));
                }

                if(recursive) {
                    if(is_file) {
                        auto off = key.rfind('/');
                        if(off != key.npos) {
                            prefixes->emplace('/' + key.substr(0, off + 1));
                        }
                        else {
                            prefixes->emplace("/");
                        }
                    }
                    else {
                        prefixes->emplace('/' + key);
                    }
                }
			}

            if(!recursive) {
                for(auto oned = List_bucket_result->FirstChildElement("CommonPrefixes");
                    oned != nullptr;
                    oned = oned->NextSiblingElement("CommonPrefixes")) {
                    auto dir = oned->FirstChild()->FirstChild()->ToText()->Value();
                    dirs.push_back(std::string("/") + dir);
                }
            }

            return is_truncated == false;
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

void bucket::_list_objects_loop(const std::string & prefix, bool recursive, std::vector<meta::content>* objects, std::vector<std::string>* folders)
{
    std::string marker;
    std::unordered_set<std::string> prefixes;

    objects->clear();
    folders->clear();

    while(!_list_objects_internal(prefix, marker, recursive, objects, folders, &marker, &prefixes)) {
        // empty block
    }

    if(recursive) {
        for(auto& f : std::move(prefixes)) {
            folders->emplace_back(std::move(f));
        }
    }
}

void bucket::list_objects(const std::string& prefix, std::vector<meta::content>* objects, std::vector<std::string>* folders)
{
    if (prefix.empty() || prefix[0] != '/') {
        throw ossexcept(ossexcept::kInvalidPath);
    }

    return _list_objects_loop(prefix.substr(1), true, objects, folders);
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

    return _list_objects_loop(prefix, recursive, objects, folders);
}

void bucket::delete_object(const std::string& obj)
{
    if(obj.empty() || obj[0] != '/') {
        throw ossexcept(ossexcept::kInvalidPath);
    }

    auto& head = _http.head();
    head.clear();

    //---------------------------- Requesting----------------------------------
    // Verb
    head.set_request("DELETE", obj);

    // Host
    head.add_host(_host);

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
    1. 204 No Content ---> Success
    2. 404 Not found ---> BucketNotFound
    3. 403 Forbidden
    ----------------------------------------------------*/
    auto& status = head.get_status();
    if(status == "204") {

    }
    else if (status == "404"){
        // TODO
        // auto oe = new bucket_error::no_such_bucket((char*)bs.data(), bs.size());
        // throw ossexcept(ossexcept::kNotFound, head.get_status_n_comment().c_str(), __FUNCTION__, oe);
    }
    else{
        throw ossexcept(ossexcept::ecode(atoi(status.c_str())), head.get_status_n_comment().c_str(), __FUNCTION__);
    }
}

bool bucket::get_object(const std::string& obj, stream::ostream& os, http::getter getter, const std::string& range/*=""*/, const std::string& unmodified_since/*=""*/)
{
    //---------------------------- Requesting----------------------------------
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("GET", obj);

    // Host
    head.add_host(_host);

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

bool bucket::put_object(const std::string& obj, stream::istream& is, http::putter putter)
{
    //---------------------------- Requesting----------------------------------
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("PUT", obj);

    head.add_content_length(is.size());

    // MIME type
    auto mime = file_system::mime(file_system::ext_name(obj));
    head.add_content_type(mime);

    // Host
    head.add_host(_host);

    //Date
    std::string date(gmt_time());
    head.add_date(date.c_str());

    // Authorization
    head.add_authorization(sign_head(_key, "PUT", "", mime, date, '/' + _name + obj));

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

const http::header::head& bucket::head_object(const std::string& obj)
{
    auto& head = _http.head();
    head.clear();

    // Verb
    head.set_request("HEAD", obj);

    // Host
    head.add_host(_host);

    //Date
    std::string date(gmt_time());
    head.add_date(date);

    // Authorization
    head.add_authorization(sign_head(_key, "HEAD", date, '/' + _name + obj));

    // Connection
    // head.add_connection("close");

    connect();
    _http.put_head();
    _http.get_head();
    disconnect();

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

