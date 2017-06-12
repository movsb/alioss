#ifndef __alioss_sign_h__
#define __alioss_sign_h__

#include <string>

namespace alioss {

// AccessKey Pair
class accesskey {
public:
	accesskey() {}
	void set_key(const char* key, const char* secret)
	{
		_key = key;
		_secret = secret;
	}

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

std::string sign(const accesskey& keysec, const std::string& str);

std::string sign(
    const accesskey& keysec,
    const std::string& verb,
    const std::string& content_md5,
    const std::string& content_type,
    const std::string& date,
    const std::string& resource
);

std::string content_md5(const void* data, int size);

} // namespace alioss


#endif // !__alioss_sign_h__

