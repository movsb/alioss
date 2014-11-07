
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

} // namespace alioss

