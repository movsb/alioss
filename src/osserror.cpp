#include <iostream>

#include <tinyxml2/tinyxml2.h>

#include "misc/color_term.h"
#include "osserror.h"

namespace alioss {

void osserr::parse(const char* xml, int size)
{
	tinyxml2::XMLDocument xmldoc;
	if (xmldoc.Parse(xml, size) == tinyxml2::XMLError::XML_NO_ERROR){
		auto node_error = xmldoc.FirstChildElement("Error");
		auto lagetfield = [&](const char* field){
			return node_error->FirstChildElement(field)->FirstChild()->ToText()->Value();
		};

		_code = lagetfield("Code");
		_msg = lagetfield("Message");
		_req_id = lagetfield("RequestId");
		_host_id = lagetfield("HostId");
	}
}

void ossexcept_stderr_dumper(ossexcept& e){
	color_term::color_term cterm;

	std::cerr << cterm(12,-1) << "-->Dumping osserr:\n" << cterm.restore();
	e.dump_osserr([](const std::string& s){
		std::cerr << "    " << s << std::endl;
	});

	std::cerr << cterm(12, -1) << "-->ExceptionCode: \n    " << cterm.restore() << (int)e.code() << std::endl;
	std::cerr << cterm(12, -1) << "-->ExceptionMsg : \n    " << cterm.restore() << e.what() << std::endl;

	std::cerr << cterm(12, -1) << "-->Dumping exception stack:\n" << cterm.restore();
	e.dump_stack([&](int i, const std::string& stk){
		std::cerr << "    " << cterm(2,-1) << i << cterm(-1,-1) << ": " << stk << std::endl;
	});

	std::cerr << cterm.restore();
}



} // namespace alioss
