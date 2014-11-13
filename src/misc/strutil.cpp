#include "strutil.h"

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

	}
}

