/*-----------------------------------------------------------------
File	: strutil
Author	: twofei <anhbk@qq.com>
Date	: Thu, Nov 13 2014
Desc	: String manipulation
-----------------------------------------------------------------*/

#ifndef __alioss_strutil_h__
#define __alioss_strutil_h__

#include <string>
#include <vector>
#include <unordered_set>
#include <codecvt>
#include <memory>

namespace alioss{
namespace strutil{

std::string strip(const std::string& s);
std::string strip(const char* s, int len=-1);

typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> U8U16Cvt;

inline std::wstring from_utf8(const std::string& s)
{
    return U8U16Cvt().from_bytes(s);
}

inline std::string to_utf8(const std::wstring& s)
{
    return U8U16Cvt().to_bytes(s);
}

std::string to_utf8(const std::string& s);

std::string encode_uri(const std::string& s);
std::string encode_uri_component(const std::string& s);

}
}


#endif //!__alioss_strutil_h__
