#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

#include "misc/color_term.h"

#ifdef _WIN32

#else 

#include <unistd.h>

#endif

#include "socket.h"

namespace alioss {

namespace socket {
	// resolver
	bool resolver::resolve(const char* host, const char* service){
		struct addrinfo hints;
		struct addrinfo* pres = nullptr;
		int res;

		free();

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		res = ::getaddrinfo(host, service, &hints, &pres);
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

	bool socket::send(const void* data, size_t sz){
		size_t n = 0;
		try{
			while (n < sz){
				size_t sent = ::send(_sockfd, (char*)data + n, sz - n, 0);
				if (sent == (size_t)-1){
					throw socketexcept(kSend, "[error] socket::send() failed", __FUNCTION__);
				}
				n += sent;
			}
		}
		catch (...){
			disconnect();
			throw;
		}

		return true;
	}

	size_t socket::recv(void* buf, size_t sz){
		bool gracefully = false;
		size_t n = 0;
		try{
			while (n < sz){
				size_t read = ::recv(_sockfd, (char*)buf + n, sz - n, 0);
				if (read == size_t(-1)){
					throw socketexcept(kRecv, "[error] socket::recv() returns -1", __FUNCTION__);
				}
				else if (read == 0){ // shutdown gracefully
					gracefully = true;
					throw socketexcept(kShutdownGracefully, "[error] socket has been shut down gracefully.", __FUNCTION__);
				}
				else{
					n += read;
				}
			}
		}
		catch (...) {
			disconnect();

			// TODO: use 'error code' + 'error msg' instead
			if (gracefully){
				return n;
			}
			else{
				throw;
			}
		}
		return n;
	}

	void socketerror_stderr_dumper(socketexcept& e)
	{
		color_term::color_term cterm;
		std::cerr << "\n" << cterm(12, -1) << "---> socketexcept:\n" << cterm(-1, -1);
		std::cerr << "    code: " << e.code() << std::endl;
		std::cerr << "    what: " << e.what() << std::endl;

		std::cerr << cterm(12, -1) << "---> socket stack:\n" << cterm(-1, -1);
		e.dump_stack([&](int i, const std::string& s){
			std::cerr << "    " << cterm(2, -1) << i << cterm(-1, -1) << ": " << s << std::endl;
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

bool http::put_body(const void* data, size_t sz)
{
	return put_body(data, sz, [](const unsigned char*,int){});
}

bool http::put_body(const void* data, size_t sz, std::function<void(const unsigned char* data, int sz)> hash)
{
	try{
		hash((unsigned char*)data, sz);
		send(data, sz);
	}
	catch (alioss::socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		throw;
	}

	return true;
}

bool http::put_body(stream::istream& is)
{
	return put_body(is, [](const unsigned char* data, int sz){});
}

bool http::put_body(stream::istream& is, std::function<void(const unsigned char* data, int sz)> hash)
{
	try{
		while (is.size()){
			unsigned char buf[1024];
			auto sz = is.read_some(buf, sizeof(buf));
			if (sz != -1){
				hash(buf, sz);
				put_body(buf, sz);
			}
		}
	}
	catch (alioss::socket::socketexcept& e){
		e.push_stack(__FUNCTION__);
		throw;
	}

	return true;
}


bool http::get_body(void* data, size_t sz)
{
	int len = get_body_len();
	char* p = reinterpret_cast<char*>(data);

	if (len != int(sz)){
		throw alioss::socket::socketexcept(alioss::socket::kBufferTooSmall, "[error] http::get_body(): buffer overflow");
	}

	return recv(p, sz)==sz;
}

bool http::get_body(stream::ostream& os)
{
	int len = get_body_len();
	int n = 0;
	while (n < len){
		unsigned char buf[1024];
		auto readn = recv(buf, sizeof(buf));
		n += os.write_some(buf, int(readn));
	}

	return true;
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
