#include "crypto.h"


#if 0

#include <iostream>

int main()
{
	crypto::cmd5 cmd5;
	cmd5.update("abc", 3);
	cmd5.finish();

	std::cout << "md5: " << cmd5.str(false) << std::endl;
	
	std::cout << "base64: " << crypto::cbase64::update("abc", 3) << std::endl;

	crypto::csha1 csha1;
	csha1.update("abc", 3);
	csha1.finish();

	std::cout << "sha1: " << csha1.str() << std::endl;

	crypto::chmac_sha1 hmac;
	const char* key = "OtxrzxIsfpFjA7SwPzILwy8Bw21TLhquhboDYROV";
	const char* text = "PUT\nODBmOGRlMDMzYTczZWY3NWE3NzA5YzdlNWYzMDQxNGM=\ntext/html\nThu, "
		"17 Nov 2005 18:49:58 GMT\nx-oss-magic:abracadabra\nx-oss-meta-author:foo@bar.com\n/oss-eample/nelson";
	hmac.update(key, strlen(key), text, strlen(text));
	std::cout << "hmac_sha1: " << hmac.str() << std::endl;

	std::cout << "base64(hmac_sha1): " << crypto::cbase64::update(hmac.digest(), 20) << std::endl;

	return 0;
}

#endif

