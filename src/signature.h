#ifndef __alioss_signature_h__
#define __alioss_signature_h__

#include <string>

#include "accesskey.h" 

namespace alioss {

std::string signature(const accesskey& keysec, const std::string& str);

std::string content_md5(const void* data, int size);

} // namespace alioss


#endif // !__alioss_signature_h__

