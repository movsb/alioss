/*---------------------------------------------------------
File	: stream
Author	: twofei <anhbk@qq.com>
Date	: Sat, Nov 8 2014
Desc	: I'm not familiar with writing a class template,
	So it's still a class with pure virtual functions
-----------------------------------------------------------*/

#pragma once

#ifndef __alioss_misc_stream_h__
#define __alioss_misc_stream_h__

namespace alioss{

namespace stream{

	class stream_size {
	public:
		virtual int size() const = 0;
	};

	class istream : public stream_size {
	public:
		virtual int read_some(unsigned char* buf, int sz) = 0;
	};

	class ostream : public stream_size {
	public:
		virtual int write_some(const unsigned char* buf, int sz) = 0;
	};

} // namespace stream

} // namespace alioss

#endif // !__alioss_misc_stream_h__
