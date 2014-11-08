#ifndef __alioss_osserror_h__
#define __alioss_osserror_h__

#include <string>
#include <vector>
#include <functional>

namespace alioss {

class osserr {
public:
	osserr(){}
	osserr(const char* xml, int size){ parse(xml, size); }
	osserr(const unsigned char* xml, int size){ parse((char*)xml, size); }

	virtual void parse(const char* xml, int size);

	const std::string& code() { return _code; }
	const std::string& msg() { return _msg; }
	const std::string& req_id() { return _req_id; }
	const std::string& host_id() { return _host_id; }

protected:
	std::string _code;
	std::string _msg;
	std::string _req_id;
	std::string _host_id;
};

class ossexcept {
	static const osserr __osserr;

public:
	enum class ecode{};

	static const ecode kNoError = ecode(0);
	static const ecode kNotFound = ecode(404);
	static const ecode kConflict = ecode(409);
	static const ecode kForbidden = ecode(403);

	ossexcept(ecode errcode, const char* errmsg, const char* errfunc=nullptr, osserr* oe=nullptr)
		: _code(errcode), _what(errmsg)
		, _osserr(oe)
	{
		if(errfunc){
			_stack.push_back(errfunc);
		}
	}

	~ossexcept(){
		if (_osserr){
			delete _osserr;
			_osserr = nullptr;
		}
	}

	void dump_stack(std::function<void(int i, const std::string&)> dumper){
		int i = _stack.size();
		for (auto& s : _stack){
			dumper(i--,s);
		}
	}

	void dump_osserr(std::function<void(const std::string&)> dumper){
		if (_osserr){
			dumper(std::string("Code     : ") + _osserr->code());
			dumper(std::string("Message  : ") + _osserr->msg());
			dumper(std::string("HostId   : ") + _osserr->host_id());
			dumper(std::string("RequestId: ") + _osserr->req_id());
		}
	}

	ecode code() { return _code; }
	std::string what() { return _what; }
	std::vector<std::string>& stack() { return _stack; }
	void push_stack(const char* func) { _stack.push_back(func); }
	const osserr& oss_err() { return _osserr ? *_osserr : __osserr; } // safe

protected:
	std::vector<std::string> _stack;
	std::string				_what;
	ecode					_code;
	osserr*					_osserr; // may be derived
};

void ossexcept_stderr_dumper(ossexcept& e);

} // namespace alioss

#endif // !__alioss_osserror_h__
