#pragma once
#include <string>
#include <sstream>
#include <iterator>
#include <vector>
#include <filesystem>
#include <map>
#include <stdlib.h>
#include <iostream>
#include "Error.h"
#define elif else if
using namespace std;

enum class Basetype {
    _none = 0,
    _any,
    _int,
    _float,
    _string,
    _ptr,
    _bool, // start data types
    _2bool,
    _3bool,
    _4bool,
    _5bool,
    _6bool,
    _7bool,
    _byte,
    _word,
    _dword,
    _qword,
    customStructure
};



enum class Tokentypes {
    null = 0,
    definition,
    redefinition,
    mold_definition,
    end,
    condition,
    condition_exec,
    condition_def,
    list_start,
    list_end,
    parameter_start,
    parameter_end,
    struct_start,
    struct_end,
    struct_variable,
    separator,
    float_separator,
    line_concat,
    _constant,
    variable,
    global_variable,
    function,
    litteral,
    _int,
    _float,
    _string,
    _char,
    ptr,
    rel,
    operation,
    type,
    global,
    local,
    lf,
    colon,
    _extern,
    _include,
    with,
    comment,
    _operator,
    child,
    _struct,
    equal,
    je,
    jz,
    jne,
    jnz,
    jg,
    jnle,
    jge,
    jnl,
    jl,
    jnge,
    jle,
    jng,
    is,
    inv
};

class constant {
public:
    string name = "";
    Tokentypes type;
    string reg = "";
    Basetype lastUse;
    bool fixedType = false;
    bool inCondition = false;
    bool isUnsigned = false;
    bool isFunctionCall = false;
    bool isSmallVar = false;
    int size = 0;
    vector<constant> parameters = {};
    constant() {};
    constant(string str, Tokentypes _type, string _reg, Basetype _Type) {
        name = str;
        type = _type;
        reg = _reg;
        lastUse = _Type;
        inCondition = false;
    }
    constant(string str, Tokentypes _type, string _reg, Basetype _Type, bool isSmall, int smallSize) {
        name = str;
        type = _type;
        reg = _reg;
        lastUse = _Type;
        inCondition = false;
        isSmallVar = isSmall;
        size = smallSize;
    }
};

struct asoperator {
    std::string name;
    vector<constant> parameters;
    bool isPrecompiled = false;
    std::string code;
    Basetype type;
    string fcode(int ident);
};

struct aschild {
    std::string name;
    vector<constant> parameters;
    bool isPrecompiled = false;
    std::string code;
    Basetype type;
    string fcode(int ident);
};

struct structure {
    string name = "";
    vector<pair<string, Basetype>> vars;
    int size = 0;
};

class token
{
public:
    int line = 0;
    string t = "";
    bool identified = false;
    Tokentypes type;
    int constant = 0;
    token* next = nullptr;
    string file;
    token(string str);
    token(string str, int line, Tokentypes type);
    token(const token&);
};

extern std::map < string, Basetype > type_dict;
extern std::map < string, Tokentypes > tokens_dict;
extern vector<asoperator> operators;
extern vector<aschild> childs;
extern string currentFile;

string fromType(Basetype t);
Basetype getType(token t);
void print_t_array(const vector<token> a, std::ostream& o = std::cout);
void fuse_symbols(vector<token>* tokens);
bool in(string a, string b[], int len);
bool in(string a, vector<string> b);
size_t in(string a, char** b, int len);
size_t in(string a, vector<token> b);
void fuse_string_litterals(vector<token>* tokens);
void fuse_structs(vector<token>* tokens);
void clean_tokens(vector<token> *tokens);
vector<token> tokenize(string file, bool isstr = false);
void identify_tokens(vector<token>* tokens, bool is_lib = false);
bool alnum(string str);
