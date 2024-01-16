#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <map>

namespace Error
{
	extern const std::string errorTypeWarn;
	extern const std::string errorTypeError;
	extern const std::string errorTypeInfo;
	extern const std::string errorTypeValid;
	extern const std::string errorTypeNormal;
	extern const int		 typeWarn;

	extern std::map<int, bool> enabledWarns;
}

void warn(int warnType, int line, std::stringstream message);