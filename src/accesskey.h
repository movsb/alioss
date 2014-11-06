#ifndef __alioss_accesskey_h__
#define __alioss_accesskey_h__

#include <string>

namespace alioss {

// AccessKey Pair
class accesskey {
public:
	accesskey(int8_t* key, int8_t* secret)
		: _key(key)
		, _secret(secret)
	{}

public:
	const std::string& key() const {
		return _key;
	}

	const std::string& secret() const {
		return _secret;
	}

protected:
	std::string _key;
	std::string _secret;
};


} // namespace alioss

#endif // !__alioss_accesskey_h__

