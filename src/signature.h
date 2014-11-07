#ifndef __alioss_signature_h__
#define __alioss_signature_h__

#include <string>

#include "accesskey.h" 

namespace alioss {

std::string signature(const accesskey& keysec, const std::string& str);

} // namespace alioss


#endif // !__alioss_signature_h__

