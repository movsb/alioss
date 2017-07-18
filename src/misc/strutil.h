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
#include <map>
#include <unordered_set>
#include <codecvt>
#include <memory>
#include <algorithm>

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

std::string encode_uri(const std::string& s);
std::string encode_uri_component(const std::string& s);

std::string make_uri(const std::string& resource, const std::map<std::string, std::string>& query);

inline void to_lower(std::string* s)
{
    std::transform(s->cbegin(), s->cend(), s->begin(), ::tolower);
}

inline std::string to_lower(const std::string& s)
{
    auto ds = s;
    to_lower(&ds);
    return ds;
}

}
}


#endif //!__alioss_strutil_h__
