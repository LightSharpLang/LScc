#include "escString.h"
#include "token.h"

/*
octal	\ooo
hexadecimal	\xhhh
*/

std::map < std::string, int > escapes = {
	{ "\n", 10 },
	{ "\\n" , 10   },
	{ "\\\\"  , 92   },
	{ "\\t" , 9    },
	{ "\\v" , 11   },
	{ "\\'" , 39   },
	{ "\\b" , 127  },
	{ "\\\"", 34   },
	{ "\\r" , 13   },
	{ "\\0" , 0    },
	{ "\\f" , 12   },
	{ "\\a" , 7    }
};

std::string formatString(std::string str) {
	std::string processedstr = str;
	std::stringstream finalstr;
	for (auto& esc : escapes) {
		size_t isin = processedstr.find(esc.first);
		while (isin != std::string::npos) {
			finalstr << processedstr.substr(0, isin);
			finalstr << std::string("\", " + to_string(esc.second) + ", \"");
			processedstr = processedstr.substr(isin + esc.first.size(), processedstr.size() - 1);
			isin = processedstr.find(esc.first);
		}
	}
	if (finalstr.str() == "") return str + ", 0";
	return finalstr.str() + processedstr;
}