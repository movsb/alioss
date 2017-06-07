#include "dirmgr.h"

namespace alioss {

dirmgr::dirmgr()
{

}

dirmgr::dirmgr(const std::string& init)
{
    push(init);
}

void dirmgr::push(const std::string& names_)
{
    auto p = names_.c_str();

    if(!*p) return;

    if(*p == '/') {
        clear();
        p++;
    }

    auto q = p;

    while(*p) {
        while(*q && *q != '/') {
            q++;
        }

        if(q == p) {
            p++;
            continue;
        }

        auto name = std::string(p, q);

        if(name == ".") {

        }
        else if(name == "..") {
            pop();
        }
        else {
            _names.push_back(std::move(name));
        }

        if(!*q) break;
        else {
            p = q + 1;
            q = p;
        }
    }
}

void dirmgr::pop()
{
    if(!_names.empty()) {
        _names.pop_back();
    }
}

void dirmgr::clear()
{
    _names.clear();
}

std::string dirmgr::str(bool endslash) const
{
    std::string s;

    if(_names.empty()) {
        return s;
    }

    for(size_t i = 0, n = _names.size() - 1; i < n; i++) {
        s += _names[i];
        s += '/';
    }

    s += _names[_names.size() - 1];

    if(endslash) {
        s += '/';
    }

    return std::move(s);
}

std::string dirmgr::file(const std::string& relative)
{
    if(!relative.empty() && relative.back() != '/') {
        dirmgr f(*this);
        auto off = relative.find_last_of('/');
        if(off != relative.npos) {
            auto dir = relative.substr(0, off - 1);
            auto nam = relative.substr(off + 1);
            f.push(dir);
            return f.str(true) + nam;
        }
        else {
            return f.str(true) + relative;
        }
    }
    else {
        return str(true) + "nul";
    }
}

std::string dirmgr::dir(const std::string& relative, bool endslash)
{
    dirmgr f(*this);
    f.push(relative);
    return f.str(endslash);
}

}
