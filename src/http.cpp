#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http.h"

namespace http{

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
		if(res != 0){
			_paddr = nullptr;
			_size = 0;
			return false;
		}

		_paddr = pres;
		while(pres){
			_size++;
			pres = pres->ai_next;
		}

		return true;
	}

// socket
bool socket::connect(const char* ip, int port){
	_sockfd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(_sockfd == -1){
		perror("socket()");
		return false;
	}

	struct sockaddr_in remote = {0};
	remote.sin_family = AF_INET;
	remote.sin_port = ::htons(port);
	remote.sin_addr.s_addr = ::inet_addr(ip);

	int rv = ::connect(_sockfd, (struct sockaddr*)&remote, sizeof(remote));
	if(rv == -1){
		::close(_sockfd);
		return false;
	}

	return true;
}

bool socket::disconnect(){
	if(_sockfd != -1){
		close(_sockfd);
		_sockfd = -1;
	}
	return true;
}

bool socket::send(const void* data, size_t sz){
	ssize_t n=0;
	while(n < sz){
		ssize_t sent = ::send(_sockfd, (char*)data+n, sz-n, 0);
		if(sent == (ssize_t)-1){
			return false;
		}

		n += sent;
	}

	return true;
}

bool socket::recv(void* buf, size_t sz){
	ssize_t n=0;
	while(n < sz){
		ssize_t read = ::recv(_sockfd, (char*)buf+n, sz-n, 0);
		if(read == ssize_t(-1)){
			return false;
		}
		else if(read == 0){
			return false;
		}
		else{
			n += read;
		}
	}

	return true;
}

} // namespace socket

bool http::put_head(){
	std::stringstream ss;
	ss << head().get_verb() << "\r\n";
	for(int i=0; i<head().size(); i++){
		auto& item = head()[i];
		ss << item.get_key() 
		   << (item.space() ? ": " : ":")
		   << item.get_val() << "\r\n";
	}
	ss << "\r\n";

	std::string request(ss.str());
	return send(request.c_str(), request.size());
}

bool http::get_head(){
	bool br = false;
	std::string line;

	head().clear();

	if((br = get_line(&line))){
		head().set_status(line.c_str());
	}
	else{
		return br;
	}

	while((br=get_line(&line))){
		if(line.size()){
			head().add(line.c_str());
		}
		else{
			break;
		}
	}
	return br;
}

bool http::get_line(std::string* line){
	char c;
	bool success=false;
	line->clear();
	while(recv(&c, 1)){
		if(c == '\r'){
			if(recv(&c, 1) && c=='\n'){
				success = true;
				break;
			}
			success = false;
			break;
		}
		else{
			*line += c;
		}
	}

	return success;
}

} // namespace http

