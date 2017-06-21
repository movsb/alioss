#ifndef __alioss_osserror_h__
#define __alioss_osserror_h__

#include <string>
#include <vector>
#include <functional>

namespace alioss {

class osserr {
public:
	osserr(){} // only this ctor can be a base class
	osserr(const char* xml, int size){ 
		parse(xml, size); 
	}
	osserr(const unsigned char* xml, int size){ 
		parse((char*)xml, size); 
	}
	osserr(void* d){
		parse(d);
	}

	void parse(const char* xml, int size);
	void parse(void* d); //xml doc

	virtual void node_handler(void* n); //xml node

	virtual void dump(std::function<void(const std::string&)> dumper);

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
	static const ecode kNotModified = ecode(304);
	static const ecode kBadRequest = ecode(400);
	static const ecode kNotFound = ecode(404);
	static const ecode kConflict = ecode(409);
	static const ecode kPreconditionFailed = ecode(412);
	static const ecode kForbidden = ecode(403);
	static const ecode kNotSpecified = ecode(-1);
	static const ecode kUnhandled = ecode(-2);
	static const ecode kInvalidArgs = ecode(-3);
	static const ecode kInvalidCheckSum = ecode(-4);
	static const ecode kTooMany = ecode(-5);
    static const ecode kInvalidPath = ecode(-6);
	static const ecode kXmlError = ecode(1);

	ossexcept(ecode errcode, const char* errmsg = "", const char* errfunc=nullptr, osserr* oe=nullptr)
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
			_osserr->dump(dumper);
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
