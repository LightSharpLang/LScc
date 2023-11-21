#pragma once
#include "token.h"
#include <algorithm>
#include "Error.h"
using namespace std;

class constant {
public:
    string name = "";
    Tokentypes type;
    string reg = "";
    Basetype lastUse;
    Basetype firstType;
    bool fixedType = false;
    bool inCondition = false;
    constant() {};
    constant(string str, Tokentypes _type, string _reg, Basetype _firstType) {
        name = str;
        type = _type;
        reg = _reg;
        firstType = firstType;
        inCondition = false;
    }
};

class condition {
public:
    string name = "";
    constant left;
    Tokentypes op;
    constant right;
    condition() {};
    condition(string str, constant _left, Tokentypes _op, constant _right) {
        name = str;
        left = _left;
        op = _op;
        right = _right;
    }
};

class argument {
public:
    string name = "";
    Tokentypes type;
    string reg = "";
    constant* ref = nullptr;
    bool is_litteral;
    argument(string str, Tokentypes _type, bool litteral) {
        name = str;
        type = _type;
        is_litteral = litteral;
    }
};

class compilation
{
private:
    int stack = 0;
    int ident = 2;
    int L = -1;
    stringstream code;
    vector<constant> constants;
    vector<condition> conditions;
    vector<string> labels;
    bool check_condi(vector<token> line, vector<token> _tokens, int* counter, bool isincondi);
    int exec_condi(vector<token> line, vector<token> _tokens, string name, int _i, bool multiline);
    void register_condi(vector<token> line);
    constant* get_registered_constant(string t);
    size_t is_registered_constant(string t);
    vector<argument> browse_argument(vector<token>& tokens);
    void register_function(vector<token> tokens);
    void compile_operation(vector<token>& _tokens);
    string get_reg();
    string browse_value(vector<token>& _tokens, bool is_global = false);
    string interpret_and_compile_var(vector<token>& _tokens);
    string interpret_and_compile(vector<token>& _tokens);
    Tokentypes detect_constant_type(token tok, vector<token>& line);
public:
    string compile_function(int function_start, vector<token>& _tokens);
};

class precompilation
{
private:
public:
    vector<coperator> operatorslib;
    vector<string> childslib;
    vector<string> childscodes;
    vector<string> functionslib;
    vector<string> functionscodes;
    bool is_operator(vector<token> _tokens, size_t index);
    bool is_child(vector<token> _tokens, size_t index);
    string precompile_lib(vector<token>& _tokens);
};

void check_externs(vector<token> tokens);
void check_pcllibs(vector<token> tokens);
vector<int> get_functions(vector<token> tokens);

extern vector<string> REG;
extern int ROspaces;
extern vector<constant> spaces;
extern string section_data;
extern vector<string> externs;
extern vector<string> pcllibs;
extern vector<int> argument_order;

vector<argument> browse_parameters(vector<token>& tokens);
bool is_int(token t);
bool is_float(token t);
bool is_string(token t);
bool is_char(token t);
bool is_list(token t);
