#include "Error.h"

const std::string	Error::errorTypeWarn	= "\x1B[35m";
const std::string	Error::errorTypeError	= "\x1B[31m";
const std::string	Error::errorTypeInfo	= "\x1B[33m";
const std::string	Error::errorTypeValid	= "\x1b[32m";
const std::string	Error::errorTypeNormal	= "\x1B[37m";
const int			Error::typeWarn			= 0;
std::map<int, bool> Error::enabledWarns = {
	{ typeWarn, true }
};

void warn(int typewarn, int line, std::stringstream message) {
	if (Error::enabledWarns[typewarn]) {
		std::cout << Error::errorTypeWarn << "Warning at line " << Error::errorTypeNormal << line << " " << message.str() << std::endl;
	}
}