#ifdef _WIN32
#include <windows.h>
#else
#endif

#include <cstring>

#include "color_term.h"

namespace color_term{

#ifdef _WIN32
	static void set_attr_windows(unsigned short a, std::ostream& os)
	{
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), (WORD)a);
	}
#else
	static void set_attr_linux(char _fg, char _bg, std::ostream& os)
	{
		int color_map[8] = {30,34,32,36,31,35,33,37};
		char buf[128]={0};
		char* p = &buf[0];
		p += sprintf(p, "\033[");
		if(_fg!=-1) p += sprintf(p, ";%d",color_map[_fg&7]);
		if(_bg!=-1) p += sprintf(p, ";%d",color_map[_bg&7]+10);
		p+=sprintf(p, "m");
		//if(_fg&8 || _bg&8) p+=sprintf(p, ";1m");

		os << buf;
	}
#endif

	color_term::color_term()
		: _def(0)
	{
#ifdef _WIN32
		::CONSOLE_SCREEN_BUFFER_INFO csbi;
		::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		_def = static_cast<unsigned short>(csbi.wAttributes);
#endif
	}

	void color_term::title(const char* tt)
	{
#ifdef _WIN32
		::SetConsoleTitleA(tt);
#endif
	}

	color color_term::restore()
	{
		return color(-1, -1, _def);
	}

	color color_term::operator()(char fg, char bg)
	{
		return color(fg, bg, _def);
	}

	void color::operator()(std::ostream& os) const
	{
#ifdef _WIN32
		unsigned short cr = _def & 0xFF00;
		cr |= _fg==-1 ? _def&0x0F : _fg&0x0F;
		cr |= _bg==-1 ? _def&0xF0 : (_bg&0x0F)<<4;
		set_attr_windows(cr, os);
#else
		set_attr_linux(_fg, _bg, os);
#endif
	}

}

std::ostream& operator<<(std::ostream& os, const color_term::color& c)
{
	c(os);
	return os;
}
