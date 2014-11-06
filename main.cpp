#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http.h"
#include "time.h"
#include "crypto/crypto.h"

using namespace alioss;

int main()
{
	http::socket::resolver resolver;
	if(!resolver.resolve("twofei.oss-cn-hangzhou.aliyuncs.com", "http")){
		return -1;
	}

	http::http sock;
	if(!sock.connect(inet_ntoa(resolver[0]->sin_addr), 80)){
		fprintf(stderr, "sock.connect() failed!\n");
		return -1;
	}
	
	auto& head = sock.head();
	head.set_verb("GET / HTTP/1.1");
	//head.add_host("oss.aliyuncs.com");
	head.add_host("twofei.oss-cn-hangzhou-a.aliyuncs.com");

	std::string gmt(gmt_time());
	head.add_date(gmt.c_str());

	std::cout << gmt.c_str() << std::endl;
	sleep(1);

	std::string id("GADlpO6YWiTjXpYr");
	std::string key("42jHE4uOxhacZbiferMYn8nADQygd4");
	std::string auth;
	auth += "OSS ";
	auth += id;
	auth += ":";

	std::string sig;
	sig += "GET\n";		// Verb
	sig += "\n";		// Content-MD5
	sig += "\n";		// Content-Type
	sig += gmt.c_str();
	sig += "\n";		// Date
	sig += "/twofei/";	// Resource

	crypto::chmac_sha1 sha1;
	sha1.update(key.c_str(), key.size(), sig.c_str(), sig.size());
	std::string sigbase64 = crypto::cbase64::update(sha1.digest(), 20);

	auth += sigbase64;

	head.add("Authorization", auth.c_str());
	
	head.add_connection("close");

	sock.put_head();
	sock.get_head();
	
	std::cout << head.get_version() << " " << head.get_status() << " " << head.get_comment() << std::endl;
	for(int i=0; i<sock.head().size(); i++){
		std::cout << sock.head()[i].get_key() << " -> " << sock.head()[i].get_val() << std::endl;
	}

	char c;
	while(sock.recv(&c, 1)){
		putchar(c);
	}

	sock.disconnect();

	return 0;
}

