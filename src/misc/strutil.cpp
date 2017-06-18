#include "strutil.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace alioss{
	namespace strutil{

		static inline void skip_ws(const char*& p){
			while (*p && (*p == ' ' || *p == '\t'))
				p++;
		}

		std::string strip(const std::string& s){
			return strip(s.c_str(), (int)s.size());
		}

		std::string strip(const char* s, int len){
			if (len == -1){
				skip_ws(s);
				if (!*s) return std::string();

				const char* t1 = s;
				const char* t2 = nullptr;

				for (;;){
					while (*t1 && *t1 != ' ' && *t1 != '\t')
						t1++;
					if (!*t1) return std::string(s);

					t2 = t1;
					skip_ws(t1);
					if (!*t1) return std::string(s, t2 - 1 - s + 1);
				}
			}
			else{
				const char* p1 = s;
				const char* p2 = s + len;

				if (p1 == p2) return std::string();

				skip_ws(p1);
				if (!*p1) return std::string();

				--p2;
				while (*p2 == ' ' || *p2 == '\t')
					--p2;

				return std::string(p1, p2 - p1 + 1);
			}
		}

        std::string to_utf8(const std::string& s)
        {
            std::string ret;

#ifdef _WIN32
            int cch = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
            if(cch > 0) {
                auto ws = std::make_unique<wchar_t[]>(cch);
                if(::MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, ws.get(), cch) > 0) {
                    ret = to_utf8(ws.get());
                }
            }
#else
            assert(0);
#endif
            return ret;
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI
        // encodeURI escapes all characters except:
        //      A-Z a-z 0-9 ; , / ? : @ & = + $ - _ . ! ~ * ' ( ) #
        std::string encode_uri(const std::string& s)
        {
            static const char hex_digits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

            std::string ret;
            ret.reserve(s.size() * 3);

            for (size_t i = 0, n = s.size(); i < n; i++) {
                int c = static_cast<unsigned char>(s[i]);
                if (c < 128 && (
                    'A' <= c && c <= 'Z'
                    || 'a' <= c && c <= 'z'
                    || '0' <= c && c <= '9'
                    || c == ';'
                    || c == ','
                    || c == '/'
                    || c == '?'
                    || c == ':'
                    || c == '@'
                    || c == '&'
                    || c == '='
                    || c == '+'
                    || c == '$'
                    || c == '-'
                    || c == '_'
                    || c == '.'
                    || c == '!'
                    || c == '~'
                    || c == '*'
                    || c == '\''
                    || c == '('
                    || c == ')'
                    || c == '#'
                    )
                )
                {
                    ret += char(c);
                }
                else {
                    ret += '%';
                    ret += hex_digits[c >> 4];
                    ret += hex_digits[c & 15];
                }
            }

            return std::move(ret);
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent
        // encodeURIComponent escapes all characters except:
        //      A-Z a-z 0-9 - _ . ! ~ * ' ( )
        std::string encode_uri_component(const std::string& s)
        {
            static const char hex_digits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

            std::string ret;
            ret.reserve(s.size() * 3);

            for(size_t i = 0, n = s.size(); i < n; i++) {
                int c = static_cast<unsigned char>(s[i]);
                if(c < 128 && (
                    'A' <= c && c <= 'Z'
                    || 'a' <= c && c <= 'z'
                    || '0' <= c && c <= '9'
                    || c == '-'
                    || c == '_'
                    || c == '.'
                    || c == '!'
                    || c == '~'
                    || c == '*'
                    || c == '\''
                    || c == '('
                    || c == ')'
                    )
                )
                {
                    ret += char(c);
                }
                else {
                    ret += '%';
                    ret += hex_digits[c >> 4];
                    ret += hex_digits[c & 15];
                }
            }

            return std::move(ret);
        }

	}
}

