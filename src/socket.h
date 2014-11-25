#ifndef __alioss_socket_h__
#define __alioss_socket_h__

#include <cstring>
#include <string>
#include <vector>
#include <functional>

#ifdef _WIN32

#include <WinSock2.h>
#include <WinInet.h>
#include <WS2tcpip.h>

#else 

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include "misc/stream.h"

namespace alioss {

namespace socket {

	typedef int socketexcept_code;

	const socketexcept_code kWSAStartupError		= -1;
	const socketexcept_code kGetAddrInfo			= -2;
	const socketexcept_code kSocket = -3;
	const socketexcept_code kConnect = -4;
	const socketexcept_code kSend = -5;
	const socketexcept_code kRecv = -6;
	const socketexcept_code kShutdownGracefully = -7;
	const socketexcept_code kContentLengthMissing = -8;
	const socketexcept_code kGetLine = -9;
	const socketexcept_code kBufferTooSmall = -10;

class socketexcept {
public:
	socketexcept(socketexcept_code code, const char* errmsg, const char* errfunc = nullptr)
		: _code(code), _what(errmsg)
	{
		if (errfunc){
			_stack.push_back(errfunc);
		}
	}

	~socketexcept(){
	}

	void dump_stack(std::function<void(int i, const std::string&)> dumper){
		int i = _stack.size();
		for (auto& s : _stack){
			dumper(i--, s);
		}
	}

	socketexcept_code code() { return _code; }
	std::string what() { return _what; }
	std::vector<std::string>& stack() { return _stack; }
	void push_stack(const char* func) { _stack.push_back(func); }

protected:
	std::vector<std::string> _stack;
	std::string				_what;
	socketexcept_code 		_code;
};

void socketerror_stderr_dumper(socketexcept& e);

#ifdef _WIN32
class wsa {
public:
	wsa()
		: _b_inited(false)
	{
	}

	~wsa()
	{
		uninit();
	}

public:
	bool init() {
		::WSAData wsa;
		if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
			throw socketexcept(kWSAStartupError, "[error] WSAStartup() failed", __FUNCTION__);
		}
		_b_inited = true;
		return true;
	}
	bool uninit() {
		if (_b_inited && ::WSACleanup() == 0){
			_b_inited = false;
			return true;
		}
		return false;
	}

protected:
	bool _b_inited;
};
#endif

class endpoint {
public:
	const std::string& ip() const { return _ip; }
	int port() const { return _port; }

	void set_ep(const char* ip, int port){
		_ip = ip;
		_port = port;
	}
private:
	std::string _ip;
	int _port;
};

class resolver{
public:
	resolver()
		: _size(0)
		, _paddr(nullptr)
	{}

	~resolver(){
		free();
	}

	bool resolve(const char* host, const char* service);

	void free() {
		_size = 0;
		if (_paddr){
			::freeaddrinfo(_paddr);
			_paddr = nullptr;
		}
	}

	size_t size() const {
		return _size;
	}

	std::string operator[](int index) {
		struct addrinfo* p = _paddr;
		while (index > 1){
			p = p->ai_next;
			index--;
		}
		return ::inet_ntoa(((sockaddr_in*)p->ai_addr)->sin_addr);
	}

protected:
	int _size;
	struct addrinfo* _paddr;
};

class socket {
public:
	socket()
		: _sockfd(-1)
		, _alive(false)
	{}

	virtual ~socket()
	{
		if (_alive){
			disconnect(); // throws
			_alive = false;
		}
	}

public:
	bool connect(const char* ip, int port);
	bool disconnect();
	bool alive() { return _alive; }
	bool send(const void* data, size_t sz);
	size_t recv(void* buf, size_t sz);

protected:
	void set_alive(bool alive){
		_alive = alive;
	}

	std::string _ip;
	int _port;
#ifdef _WIN32
	SOCKET _sockfd;
#else
	int _sockfd;
#endif
	bool _alive;
};

} // namespace socket

namespace http {

namespace header {

	typedef const char* const FIELD;
	static FIELD kContentLength = "Content-Length";
	static FIELD kContentType = "Content-Type";
	static FIELD kIfModifiedSince = "If-Modified-Since";
	static FIELD kIfUnmodifiedSince = "If-Unmodified-Since";
	static FIELD kIfMatch = "If-Match";
	static FIELD kIfNoneMatch = "If-None-Match";

class item
{
public: // ctors & dtors
	template<class Tk, class Tv>
	item(const Tk& key, const Tv& val, bool space=true)
		: _key(key)
		, _val(val)
		, _space(space)
	{}

public:
	const std::string& get_key() const {
		return _key;
	}

	const std::string& get_val() const {
		return _val;
	}

	template<class T>
	void set_key(const T& t){
		_key = t;
	}

	template<class T>
	void set_val(const T& t){
		_val = t;
	}

	template<class Tk, class Tv>
	bool set_keyval(const Tk& key, const Tv& val){
		set_key(key);
		set_val(val);
		return true;
	}

	bool space() const {
		return _space;
	}

protected:
	bool _space;
	std::string _key;
	std::string _val;
};

class head {
public:
	void clear() {
		_items.clear();
	}

	bool empty() {
		return _items.empty();
	}

	size_t size() {
		return _items.size();
	}

	const item& operator[](int i) {
		return _items[i];
	}

	template<class T>
	bool set_verb(const T& verb){
		_verb = verb;
		return true;
	}

	const std::string& get_verb() const {
		return _verb;
	}

	bool set_status(const char* st){
		auto p = st;

		st = p;
		while(*p!=' ') p++;
		_version = std::string(st, p-st);
		while(*p==' ') p++;

		st = p;
		while(*p!=' ') p++;
		_status = std::string(st, p-st);
		while(*p== ' ') p++;

		st = p;
		while(*p) p++;
		_comment = std::string(st, p-st);
	
		return true;
	}

	const std::string& get_version() {
		return _version;
	}

	const std::string& get_comment() {
		return _comment;
	}

	const std::string& get_status() {
		return _status;
	}

	std::string get_status_n_comment() {
		return _status + " " + _comment;
	}

	bool add(const char* keyval, bool space=true){
		auto pos1 = strchr(keyval, ':');
		
		std::string k(keyval, pos1-0);

		pos1++;
		while(*pos1==' ')
			pos1++;
		std::string v(pos1);

		return add(k, v, space);
	}

	template<class Tk, class Tv>
	bool add(const Tk& key, const Tv& val, bool space=true){
		_items.push_back(item(key, val, space));
		return true;
	}

	template<class T>
	bool add_host(const T& val){
		return add("Host", val);
	}

	template<class T>
	bool add_user_agent(const T& val){
		return add("User-Agent", val);
	}

	template<class T>
	bool add_accept(const T& val){
		return add("Accept", val);
	}

	template<class T>
	bool add_accept_language(const T& val){
		return add("Accept-Language", val);
	}
	
	template<class T>
	bool add_accept_encoding(const T& val){
		return add("Accept-Encoding", val);
	}

	template<class T>
	bool add_connection(const T& val){
		return add("Connection", val);
	}

	template<class T>
	bool add_date(const T& val){
		return add("Date", val);
	}
	
	template<class T>
	bool add_authorization(const T& val){
		return add("Authorization", val);
	}

	bool add_content_length(size_t len){
		char buf[32];
		sprintf(buf, "%u", len);
		return add("Content-Length",buf, true);
	}

	template<class T>
	bool add_content_type(const T& type){
		return add("Content-Type", type);
	}

	template<class T>
	bool add_content_disposition(const T& val){
		return add("Content-Disposition", val);
	}

	template<class T>
	bool add_content_encoding(const T& val){
		return add("Content-Encoding", val);
	}

	template<class T>
	bool remove(const T& key){
		for(auto it=_items.begin(); it!=_items.end();){
			if(it->get_key() == key){
				it = _items.erase(it);
			}
			else{
				it++;
			}
		}
		
		return true;
	}

	template<class T>
	bool get(const T& key, std::string* pval){
		for(auto& item : _items){
			if(item.get_key() == key){
				*pval = item.get_val();
				return true;
			}
		}
		return false;
	}

	void dump(std::function < bool( // return false to stop enumeration
		std::vector<item>::size_type i,  // start from 1
		decltype(static_cast<item*>(0)->get_key())& k,
		decltype(static_cast<item*>(0)->get_val())& v) > dumper) const;

protected:
	std::string _verb;
	std::string _version;
	std::string _status;
	std::string _comment;
	std::vector<item> _items;
};

} // namespace header

const char* const scrlf = "\r\n";

class http : public socket::socket
{
public:
	header::head& head() {
		return _head;
	}

	bool put_head();
	bool get_head();
	int  get_body_len();
	bool put_body(const void* data, size_t sz);
	bool put_body(const void* data, size_t sz, std::function<void(const unsigned char* data, int sz)> hash);
	bool put_body(stream::istream& is);
	bool put_body(stream::istream& is, std::function<void(const unsigned char* data, int sz)> hash);
	bool get_body(void* data, size_t sz);
	bool get_body(stream::ostream& os);

	bool get_line(std::string* line, bool crlf=false);

protected:
	header::head _head;	
};

class str_body_ostream : public stream::ostream {
public:
	virtual int size() const override {
		return static_cast<int>(_buffer.size());
	}

	virtual int write_some(const unsigned char* buf, int sz) override
	{
		_buffer += std::string((char*)buf, sz);
		return sz;
	}

public:
	unsigned char* data() {
		return (unsigned char*)_buffer.c_str();
	}

protected:
	std::string _buffer;
};

} // namespace http

} // namespace alioss

#endif // !__alioss_socket_h__

