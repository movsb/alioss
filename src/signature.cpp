
#include "crypto/crypto.h"
#include "signature.h"

namespace alioss{

std::string signature(const accesskey& keysec, const std::string& msg)
{
	using namespace crypto;

	chmac_sha1 hmac_sha1;
	hmac_sha1.update(keysec.secret().c_str(), keysec.secret().size(), msg.c_str(), msg.size());

	std::string b64str(cbase64::update(hmac_sha1.digest(), 20));

	std::string sig("OSS ");
	sig += keysec.key();
	sig += ":";
	sig += b64str;

	return sig;
}

std::string content_md5(const void* data, int size)
{
	using namespace crypto;
	cmd5 md5;
	md5.update(data, size);
	md5.finish();

	return cbase64::update(md5.digest(), 16);
}


} // namespace alioss

