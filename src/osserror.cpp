#include <iostream>

#include <tinyxml2/tinyxml2.h>

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
	std::cerr << "-->Dumping osserr:\n";
	e.dump_osserr([](const std::string& s){
		std::cerr << "    " << s << std::endl;
	});

	std::cerr << "-->ExceptionCode: \n    " << (int)e.code() << std::endl;
	std::cerr << "-->ExceptionMsg : \n    " << e.what() << std::endl;

	std::cerr << "-->Dumping exception stack:\n";
	e.dump_stack([](int i, const std::string& stk){
		std::cerr << "    " << i << ": " << stk << std::endl;
	});
}



} // namespace alioss
