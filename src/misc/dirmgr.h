#pragma once

#include <vector>
#include <string>

namespace alioss {

class dirmgr
{
public:
    dirmgr();
    dirmgr(const std::string& init);

public:
    void push(const std::string& names);
    void pop();
    void clear();
    std::string str(bool endslash = false) const;
    std::string file(const std::string& relative);
    std::string dir(const std::string& relative, bool endslash);

protected:
    std::vector<std::string> _names;
};

}
