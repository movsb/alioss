#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

#include "service.h"

using namespace alioss;

int main()
{
	socket::wsa_instance wsa;
	service::service svc;
	svc.set_key("GADlpO6YWiTjXpYr", "42jHE4uOxhacZbiferMYn8nADQygd4");
	svc.connect();
	svc.query();
	svc.disconnect();

	return 0;
}

