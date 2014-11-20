#ifdef _WIN32
#include <windows.h>
#else
#endif

#include "color_term.h"

namespace color_term{

	static void set_attr(unsigned short a)
	{
#ifdef _WIN32
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), (WORD)a);
#endif
	}

	color_term::color_term()
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

	void color::operator()() const
	{
		unsigned short cr = _def & 0xFF00;
		cr |= _fg==-1 ? _def&0x0F : _fg&0x0F;
		cr |= _bg==-1 ? _def&0xF0 : (_bg&0x0F)<<4;

		set_attr(cr);
	}

}

std::ostream& operator<<(std::ostream& os, const color_term::color& c)
{
	c();
	return os;
}
