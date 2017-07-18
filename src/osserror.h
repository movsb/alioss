#ifndef __alioss_osserror_h__
#define __alioss_osserror_h__

#include <string>
#include <iostream>

#include "socket.h"

namespace alioss {

class osserr
{
public:
    friend std::ostream& operator<<(std::ostream& os, const osserr& e);
    static osserr handle(const http::header::head& head, const std::string& body = {});

    osserr(){}

    osserr(const std::string& head, const std::string& body)
        : _head(head)
        , _body(body)
    { }

protected:
    std::string _head;
    std::string _body;
};

std::ostream& operator<<(std::ostream& os, const osserr& e);

} // namespace alioss

#endif // !__alioss_osserror_h__
