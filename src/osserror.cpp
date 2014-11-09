#include <iostream>

#include <tinyxml2/tinyxml2.h>

#include "misc/color_term.h"
#include "osserror.h"

namespace alioss {

void osserr::parse(const char* xml, int size)
{
	tinyxml2::XMLDocument xmldoc;
	if (xmldoc.Parse(xml, size) == tinyxml2::XMLError::XML_NO_ERROR){
		parse(&xmldoc);
	}
}

void osserr::parse(void* d)
{
	auto doc = reinterpret_cast<tinyxml2::XMLDocument*>(d);

	if (doc->Error()) return;

	auto node_error = doc->FirstChildElement("Error");

	// All Standard nodes are parsed firstly and removed
	auto lagetfield = [&](const char* field)->std::string{
		auto nodeele = node_error->FirstChildElement(field);
		auto str = std::string(nodeele->FirstChild()->ToText()->Value());
		node_error->DeleteChild(nodeele);
		return str;
	};

	_code = lagetfield("Code");
	_msg = lagetfield("Message");
	_req_id = lagetfield("RequestId");
	_host_id = lagetfield("HostId");

	// Cycle through all others nodes to
	// the custom-defined node_handler

	for (auto node = node_error->FirstChild();
		node != nullptr;
		node = node->NextSibling())
	{
		node_handler(node);
	}
}

void osserr::node_handler(void* n)
{
	// nothing to do
}

void osserr::dump(std::function<void(const std::string&)> dumper)
{
	dumper(std::string("Code     : ") + code());
	dumper(std::string("Message  : ") + msg());
	dumper(std::string("HostId   : ") + host_id());
	dumper(std::string("RequestId: ") + req_id());
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
