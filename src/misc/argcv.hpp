#include <cctype>
#include <vector>
#include <string>
#include <iostream>

namespace snippets {
    class Argcv
    {
    public:
        Argcv()
        {
            clear();
        }

        ~Argcv()
        {
            clear();
        }

        void clear()
        {
            _argv.clear();
        }

        int argc() const
        {
            return _argv.empty() ? 0 : (int)_argv.size() - 1;
        }

        char* const* argv() const
        {
            return _argv.data();
        }

        bool parse(const char* s)
        {
            clear();

            if (s == nullptr) {
                return true;
            }

            _str = s;

            auto p = &_str[0];

            while (*p) {
                while (std::isspace(*p))
                    p++;
                if (*p == '\0') {
                    break;
                }

                if (*p == '\'' || *p == '"') {
                    auto c = *p++;
                    _argv.push_back(p);
                    while (*p && *p != c)
                        p++;
                    if (!*p) {
                        clear();
                        return false;
                    }
                    *p++ = '\0';
                    if (*p && !std::isspace(*p)) {
                        clear();
                        return false;
                    }
                }
                else {
                    _argv.push_back(p);
                    while (*p && !std::isspace(*p))
                        p++;
                    if (std::isspace(*p))
                        *p++ = '\0';
                }
            }

            _argv.push_back(p);

            return true;
        }

    protected:
        std::string _str;
        std::vector<char*> _argv;
    };
}

