#include "escString.h"
#include "token.h"
using namespace std;

/*
octal	\ooo
hexadecimal	\xhhh
*/

map < string, int > escapes = {
	{ "\n"		, 10    },
	{ "\\n"		, 10	},
	{ "\\\\"	, 92    },
	{ "\\t"		, 9     },
	{ "\\v"		, 11    },
	{ "\\'"		, 39    },
	{ "\\b"		, 127   },
	{ "\\\""	, 34    },
	{ "\\r"		, 13    },
	{ "\\0"		, 0     },
	{ "\\f"		, 12    },
	{ "\\a"		, 7     }
};

string formatString(string str) {
	string processedstr = str;
	stringstream finalstr;
	for (auto& esc : escapes) {
		size_t isin = processedstr.find(esc.first);
		while (isin != string::npos) {
			finalstr << processedstr.substr(0, isin);
			finalstr << string("\", " + to_string(esc.second) + ", \"");
			processedstr = processedstr.substr(isin + esc.first.size(), processedstr.size() - 1);
			isin = processedstr.find(esc.first);
		}
	}
	if (finalstr.str() == "") return str + ", 0";
	return finalstr.str() + processedstr;
}