#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include "compilation.h"*
using namespace std;

enum class LsccArg {
    none = 0,
    i,
    o,
    cc,
    f,
    n,
    s
};