#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32

#else 

#include <unistd.h>

#endif

#include "misc/strutil.h"

#include "socket.h"

#undef min
#undef max

namespace alioss {

namespace socket {
	// resolver
	bool resolver::resolve(const std::string& host, const std::string& service){
		struct addrinfo hints;
		struct addrinfo* pres = nullptr;
		int res;

		free();

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		res = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &pres);
		if (res != 0){
			_paddr = nullptr;
			_size = 0;

			throw socketexcept(kGetAddrInfo, "[error] getaddrinfo() failed.", __FUNCTION__);
		}

		_paddr = pres;
		while (pres){
			_size++;
			pres = pres->ai_next;
		}

		return true;
	}
	
	// socket
	bool socket::connect(const char* ip, int port){
		_sockfd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); if (_sockfd == -1){
			throw socketexcept(kSocket, "[error] socket error.", __FUNCTION__);
		}

		struct sockaddr_in remote = { 0 };
		remote.sin_family = AF_INET;
		remote.sin_port = ::htons(port);
		remote.sin_addr.s_addr = ::inet_addr(ip);

		int rv = ::connect(_sockfd, (struct sockaddr*)&remote, sizeof(remote));
		if (rv == -1){
#ifdef _WIN32
			::closesocket(_sockfd);
#else
			::close(_sockfd);
#endif
			throw socketexcept(kConnect, "[error] connection to host failed.", __FUNCTION__);
		}

		set_alive(true);
		return true;
	}

	bool socket::disconnect(){
		if (_sockfd != -1){
#ifdef _WIN32
			::closesocket(_sockfd);
#else
			::close(_sockfd);
#endif
			_sockfd = -1;
		}

		set_alive(false);
		return true;
	}

	bool socket::send(const void* data, size_t sz, putter cb){
		size_t n = 0;
		try{
			while (n < sz){
                int slice = std::min(int(sz - n), 102400);
				size_t sent = ::send(_sockfd, (char*)data + n, slice, 0);
				if (sent == (size_t)-1){
					throw socketexcept(kSend, "[error] socket::send() failed", __FUNCTION__);
                } else if(sent == 0) {
                    throw socketexcept(kShutdownGracefully, "[error] socket has been shut down gracefully.", __FUNCTION__);
                } else {
                    if(cb) cb((unsigned char*)data + n, (int)sent, sz, n+sent);
                    n += sent;
                }
			}
		}
		catch (...){
			disconnect();
			throw;
		}

		return true;
	}

	size_t socket::recv(void* buf, size_t sz, getter cb){
		size_t n = 0;
		try{
			while (n < sz){
                int slice = std::min(int(sz - n), 102400);
				size_t read = ::recv(_sockfd, (char*)buf + n, slice, 0);
				if (read == size_t(-1)){
					throw socketexcept(kRecv, "[error] socket::recv() returns -1", __FUNCTION__);
				}
				else if (read == 0){ // shutdown gracefully
					throw socketexcept(kShutdownGracefully, "[error] socket has been shut down gracefully.", __FUNCTION__);
				}
				else{
                    if(cb) cb((unsigned char*)buf + n, (int)read, sz, n+read);
					n += read;
				}
			}
		}
		catch (...) {
			disconnect();
            throw;
		}
		return n;
	}

	void socketerror_stderr_dumper(socketexcept& e)
	{
		std::cerr << "\n" << "---> socketexcept:\n";
		std::cerr << "    code: " << e.code() << std::endl;
		std::cerr << "    what: " << e.what() << std::endl;

		std::cerr << "---> socket stack:\n";
		e.dump_stack([&](int i, const std::string& s){
			std::cerr << "    " << i << ": " << s << std::endl;
		});
	}


} // namespace socket

namespace http {

bool http::put_head(){
	// Oops: I should use `send' directly
	std::stringstream ss;
	ss << head().get_verb() << scrlf;
	for (int i = 0, c = (int)head().size(); i < c; i++){
		auto& item = head()[i];
		ss << item.get_key()
			<< (item.space() ? ": " : ":")
			<< item.get_val() << scrlf;
	}
	ss << scrlf;

	std::string request(ss.str());
	return send(request.c_str(), request.size());
}

bool http::get_head(){
	std::string line;

	_head.clear();

	get_line(&line, true);
	_head.set_status(line.c_str());

	// line is cleared each time we call to get_line()
	while (get_line(&line, true) && line.size()){
		_head.add(line.c_str());
	}
	return true;
}

int http::get_body_len()
{
	std::string len;
	if (_head.get(header::kContentLength, &len)){
		return ::atoi(len.c_str());
	}

	throw alioss::socket::socketexcept(alioss::socket::kContentLengthMissing, "http::get_body_len() fails due to no such field.", __FUNCTION__);
}

// crlf is discarded
bool http::get_line(std::string* line, bool crlf){
	char c;
	bool success = false;
	line->clear();
	while (recv(&c, 1)==1){
		if (c == '\n'){
			success = true;
			break;
		}
		else{
			if (c == '\r' && crlf){
				if (recv(&c, 1)==1 && c=='\n'){
					success = true;
					break;
				}

				throw alioss::socket::socketexcept(alioss::socket::kGetLine, "[error] http::get_line() fails due to '\n' missing.", __FUNCTION__);
			}
			else{
				*line += c;
			}
		}
	}

	return success;
}

bool http::put_body(const void* data, size_t sz, putter cb)
{
	try{
		send(data, sz, cb);
	}
	catch (alioss::socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		throw;
	}

	return true;
}

bool http::put_body(stream::istream& is, putter cb)
{
	try{
        int n = 0;
        int totalsize = is.size();
		while (is.size()){
			unsigned char buf[102400];
            int slice = std::min(is.size(), (int)sizeof(buf));
			auto sz = is.read_some(buf, slice);
			if (sz != -1){
                put_body(buf, sz, [&](const unsigned char* data, int size, int total, int transferred) {
                    if(cb) {
                        cb(data, size, totalsize, n+sz);
                    }
                });
                n += sz;
			}
		}
	}
	catch (alioss::socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		throw;
	}

	return true;
}


bool http::get_body(void* data, size_t sz, getter cb)
{
	int len = get_body_len();
	char* p = reinterpret_cast<char*>(data);

	if (len > int(sz)){
		throw alioss::socket::socketexcept(alioss::socket::kBufferTooSmall, "[error] http::get_body(): buffer overflow");
	}

    return recv(p, sz, cb) == sz;
}

bool http::get_body(stream::ostream& os, getter cb)
{
	int len = get_body_len();
	int n = 0;
	while (n < len){
		unsigned char buf[102400];
        size_t slice = std::min(len - n, (int)sizeof(buf));
        size_t readn = 0;
        readn = recv(buf, slice, [&](const unsigned char* data, int size, int total, int transferred) {
            if(cb) {
                cb(data, size, len, n+readn);
            }
        });
        if(os.write_some(buf, int(readn)) != readn) {

        }
        n += readn;
	}

	return true;
}


void header::head::set_request(const std::string & method, const std::string & resource, const std::map<std::string, std::string>& query, const std::string & version)
{
    std::ostringstream oss;

    oss << method << ' ';

    oss << strutil::encode_uri(resource);

    if(!query.empty()) {
        std::string qs;

        for(const auto& kv : query) {
            auto ek = strutil::encode_uri_component(kv.first);
            auto ev = strutil::encode_uri_component(kv.second);
            qs += '&' + ek + '=' + ev;
        }

        qs[0] = '?';

        oss << qs;
    }

    oss << ' ';
    oss << version;

    _verb = oss.str();
}

void header::head::dump(std::function<bool(
	std::vector<item>::size_type i,
	decltype(static_cast<item*>(0)->get_key())& k, 
	decltype(static_cast<item*>(0)->get_val())& v) > dumper) const
{
	std::vector<item>::size_type i = 0;
	for (auto& e : _items){
		dumper(++i, e.get_key(), e.get_val());
	}
}

} // namespace http

} // namespace alioss
