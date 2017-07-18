#include <iostream>

#include <tinyxml2/tinyxml2.h>

#include "osserror.h"

namespace alioss {

osserr osserr::handle(const http::header::head & head, const std::string & body)
{
    // Not an error.
    if(head.get_status()[0] == '2') {
        return {};
    }

    return osserr(head.get_status_n_comment(), body);
}

std::ostream & operator<<(std::ostream & os, const osserr& e)
{
    os << "HEAD: \n" << e._head << std::endl;
    if(!e._body.empty()) {
        os << "BODY:\n" << e._body << std::endl;
    }
    return os;
}

} // namespace alioss
