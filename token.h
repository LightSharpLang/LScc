#pragma once
#include <string>
#include <sstream>
#include <iterator>
#include <vector>
#include <map>
#include <stdlib.h>
#include <iostream>
#include "Error.h"
#define elif else if
using namespace std;

struct coperator {
    std::string name;
    vector<std::string> parameters;
    std::string code;
    string fcode(int ident);
};

enum class Tokentypes {
    null = 0,
    definition,
    end,
    condition,
    condition_exec,
    condition_def,
    list_start,
    list_end,
    parameter_start,
    parameter_end,
    separator,
    float_separator,
    line_separator,
    line_concat,
    _constant,
    variable,
    global_variable,
    function,
    litteral,
    parameter,
    _int,
    _float,
    _string,
    _char,
    ptr,
    operation,
    type,
    unidentifyed_type,
    global,
    local,
    lf,
    colon,
    _extern,
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
    inv,
    strret,
    strterm,
    strback,
};

extern std::map < string, Tokentypes > tokens_dict;
extern vector<coperator> operators;

class token
{
public:
    int line = 0;
    string t = "";
    bool identified = false;
    Tokentypes type;
    int constant = 0;

    token(string str);
};

void fuse_symbols(vector<token>* tokens);
bool in(string a, string b[], int len);
size_t in(string a, char** b, int len);
void fuse_string_litterals(vector<token>* tokens);
void clean_tokens(vector<token> *tokens);
vector<token> tokenize(string file, bool isstr = false);
void identify_tokens(vector<token>* tokens, bool is_lib = false);
bool alnum(string str);