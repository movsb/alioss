/*--------------------------------------------------------------
File	: color_term\.(h|cpp)
Author	: twofei <anhbk@qq.com>
Copy	: (C) 2014-2038
Date	: Sun, Nov 9 2014
Blog	: http://www.cnblogs.com/memset/color_terminal.html
Desc	: Free software, free to cp/ed/mv. If you have a problem
	using it, contact me anytime. Bugs to my email, plz.

Bit color table:
		+--------------------------------------+
		|     1     |    1   |    1   |    1   |
		+--------------------------------------+
		| (intense) |   red  |  green |  blue  |
		+--------------------------------------+

		Samples:
			Red		: 4
			Green	: 2
			Blue	: 1
			White	: 7
			Black	: 0
			Yellow	: 6 (Red+Green)
			Magenta	: 5 (Red+Blue)
			Cyan	: 3 (Green+Blue)

		Intensity:
			If you want a color to be highlighted, plus 8 to it.
--------------------------------------------------------------*/

#ifndef ___color_term_h__
#define ___color_term_h__

#include <ostream>

namespace color_term{

	class color{
	public:
		color(char fg, char bg, unsigned short def)
			: _fg(fg), _bg(bg), _def(def){}
		void operator()();
	private:
		char _fg,_bg;
		unsigned short _def;
	};

	class color_term {
	public:
		color_term();
		color restore();
		color operator()(char fg, char bg);
		void title(const char* tt);
	private:
		unsigned short _def;
	};

} // namespace color_term

std::ostream& operator<<(std::ostream& os, color_term::color& c);

#endif // !___color_term_h__
