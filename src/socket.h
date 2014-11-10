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

class wsa_instance {
public:
	wsa_instance()
		: _b_inited(false)
	{
		init();
	}

	~wsa_instance()
	{
		uninit();
	}

public:
	bool init() {
		::WSAData wsa;
		if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
			throw std::runtime_error("[error] WSAStartup()");
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

class endpoint {
public:
	const std::string& ip() { return _ip; }
	int port() { return _port; }

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

class item
{
public: // ctors & dtors
	item(const char* key, const char* val, bool space=true)
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

	void set_key(const char* key) {
		_key = key;
	}

	void set_val(const char* val) {
		_val = val;
	}

	bool set_keyval(const char* keyval);

	bool set_keyval(const char* key, const char* val){
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

	bool set_verb(const char* verb){
		_verb = verb;
		return true;
	}

	std::string get_verb() const {
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

		return add(k.c_str(), v.c_str(), space);
	}

	bool add(const char* key, const char* val, bool space=true){
		_items.push_back(item(key, val, space));
		return true;
	}

	bool add_host(const char* val){
		return add("Host", val);
	}

	bool add_user_agent(const char* val){
		return add("User-Agent", val);
	}

	bool add_accept(const char* val){
		return add("Accept", val);
	}

	bool add_accept_language(const char* val){
		return add("Accept-Language", val);
	}
	
	bool add_accept_encoding(const char* val){
		return add("Accept-Encoding", val);
	}

	bool add_connection(const char* val){
		return add("Connection", val);
	}

	bool add_date(const char* val){
		return add("Date", val);
	}
	
	bool add_authorization(const char* val){
		return add("Authorization", val);
	}

	bool add_content_length(size_t len){
		char buf[32];
		sprintf(buf, "%u", len);
		return add("Content-Length", buf);
	}

	bool add_content_type(const char* type){
		return add("Content-Type", type);
	}

	bool add_content_disposition(const char* val){
		return add("Content-Disposition", val);
	}

	bool add_content_encoding(const char* val){
		return add("Content-Encoding", val);
	}

	bool remove(const char* key){
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

	bool get(const char* key, std::string* pval){
		for(auto& item : _items){
			if(item.get_key() == key){
				*pval = item.get_val();
				return true;
			}
		}
		return false;
	}			

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

