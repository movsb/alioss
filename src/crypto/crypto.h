#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <string>

extern "C" {
#include "md5.h"
#include "base64_encode.h"
#include "sha1.h"
#include "hmac_sha1.h"
}

namespace crypto{

class cmd5 {
public:
	cmd5(){
		init();
	}

	void init(){
		::md5_init(&_state);
	}

	void update(const void* data, size_t sz){
		::md5_append(&_state, (md5_byte_t*)data, int(sz));
	}

	void finish(){
		::md5_finish(&_state, &_digest[0]);
	}

	const md5_byte_t* digest(){
		return _digest;
	}

	std::string str(bool uppercase=true){
		char s[33];
		for(int i=0; i<16; i++){
			sprintf(&s[0]+i*2, uppercase ? "%02X" : "%02x", _digest[i]);
		}

		return s;
	}

protected:
	md5_state_s _state;
	md5_byte_t _digest[16];
};

class cbase64{
public:
	static std::string update(const void* data, size_t sz){
		size_t outsz=0;
		char* s = ::base64_encode((unsigned char*)data, sz, &outsz);
		
		std::string ss(s, outsz);
		::free(s);

		return ss;
	}
};

class csha1 {
public:
	csha1(){
		SHA1Reset(&_ctx);
	}

	void init(){
		::SHA1Reset(&_ctx);
	}

	void update(const void* data, size_t sz){
		::SHA1Input(&_ctx, (uint8_t*)data, (unsigned int)sz);
	}

	void finish(){
		::SHA1Result(&_ctx, &_digest[0]);
	}

	std::string str(){
		char s[41];
		for(int i=0; i<20; i++){
			sprintf(&s[0]+i*2, "%02X", _digest[i]);
		}

		return s;
	}

protected:
	SHA1Context _ctx;
	uint8_t _digest[20];
};

class chmac_sha1 {
public:
	void update(const void* key, int keylen, const void* msg, int msglen){
		::hmac_sha1((unsigned char*)key, keylen, (unsigned char*)msg, msglen, &_digest[0]);
	}

	std::string str(){
		char s[41];
		for(int i=0; i<20; i++){
			sprintf(&s[0]+i*2, "%02X", _digest[i]);
		}
		return s;
	}

	unsigned char* digest() {
		return &_digest[0];
	}
protected:
	unsigned char _digest[20];
};

} // namespace crypto

