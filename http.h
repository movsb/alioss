#include <cstring>

namespace http {

namespace header {

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

namespace socket {
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
		if(_paddr){
			::freeaddrinfo(_paddr);
			_paddr = nullptr;
		}
	}

	size_t size() const {
		return _size;
	}

	struct sockaddr_in* operator[](int index) {
		struct addrinfo* p = _paddr;
		while(index > 1){
			p = p->ai_next;
			index--;
		}
		return (struct sockaddr_in*)p->ai_addr;
	}
			
protected:
	int _size;
	struct addrinfo* _paddr;
};

class socket {
public:
	bool connect(const char* ip, int port);
	bool disconnect();
	bool send(const void* data, size_t sz);
	bool recv(void* buf, size_t sz);

protected:
	std::string _ip;
	int _port;
	int _sockfd;
};

} // namespace socket

class http : public socket::socket
{
public:
	header::head& head() {
		return _head;
	}

	bool put_head();
	bool get_head();

protected:
	bool get_line(std::string* line);

protected:
	header::head _head;	
};

} // namespace http

