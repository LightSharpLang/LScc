#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
using namespace std;

enum LsccArg {
    none = 0,
    i,
    o,
    cc,
    f,
    n,
    s,
    p,
    info
};

extern map<LsccArg, string> Args;