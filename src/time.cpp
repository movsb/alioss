#include "time.h"

namespace alioss {

std::string gmt_time(){
	time_t rawtime;
	tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);

	char str[32];
	strftime(str, 32, "%a, %d %b %Y %H:%M:%S GMT", ptm);
	return str;
}

}

