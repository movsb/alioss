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

#include <string>
#include <fstream>

#include "strutil.h"

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

    class file_ostream : public stream::ostream {
    public:
        virtual int size() const override{
            return static_cast<int>(_fstm.width()); // ???
        }
        virtual int write_some(const unsigned char* buf, int sz) override{
            _fstm.write((char*)buf, sz);
            return sz;
        }

    public:
        ~file_ostream(){
            close();
        }

        bool open(const std::string& file){
            _fstm.open(strutil::from_utf8(file), std::ios_base::ate | std::ios_base::binary);
            return _fstm.is_open();
        }

        void close() {
            _fstm.close();
        }

    protected:
        std::ofstream _fstm;
};

} // namespace stream

} // namespace alioss

#endif // !__alioss_misc_stream_h__
