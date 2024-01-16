#pragma once
#include "token.h"
#include "escString.h"
#include "preprocessor.h"
#include "lscc.h"
#include <algorithm>
#include <array>
using namespace std;

enum class CallingConvention {
    SysVi386,
    SysV,
    M64,
    cdelc
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

class compilation
{
private:
    int stack = 0;
    stringstream conditionCode;
    vector<condition> conditions;
    vector<constant> extConstants;
    string currentFname;
    void swap();
    bool check_condi(vector<token> line, vector<token> _tokens, int* counter, bool isincondi);
    int exec_condi(vector<token> line, vector<token> _tokens, string name, int _i, bool multiline, bool inverted);
    void register_condi(vector<token> line);
    constant* get_registered_constant(string t);
    size_t is_registered_constant(string t);
    vector<constant> browse_argument(vector<token>& tokens, bool ignoreConstantError = false);
    void register_function(vector<token> tokens);
    void compile_operation(vector<token>& _tokens);
    string get_reg();
    string browse_value(vector<token>& _tokens, bool is_global = false);
    string interpret_and_compile_var(vector<token>& _tokens);
    string interpret_and_compile(vector<token>& _tokens);
    Tokentypes detect_constant_type(token tok, vector<token>& line);
    void getStack(vector<token>& _tokens);
    string compile_mold(bool isMold, bool isChild, bool isOperator, int offset, vector<token>& _tokens, vector<constant> passedParameters);
public:
    int L = -1;
    vector<token> __tokens;
    stringstream code;
    vector<constant> constants;
    vector<string> labels;
    compilation() {};
    compilation(compilation& c);
    compilation operator =(const compilation& nc);
    string compile_function(int function_start, vector<token>& _tokens);
    void add_external_constant(constant c);
};

class precompilation
{
private:
public:
    vector<asoperator> operatorslib;
    vector<aschild> childslib;
    bool is_operator(vector<token> _tokens, size_t index);
    bool is_child(vector<token> _tokens, size_t index);
    string precompile_lib(vector<token>& _tokens);
};

extern vector<string> REG;
extern int ROspaces;
extern vector<constant> spaces;
extern string section_data;
extern vector<constant> externs;
extern vector<string> includes_f;
extern vector<string> pcllibs;
extern vector<int> argument_order;
extern filesystem::path WorkingDirectory;
extern CallingConvention convention;
extern class preprocessor preproc;
extern int architecture;

compilation compile_file(string file, string& section_text, compilation c);
void check_externs(vector<token> tokens);
void check_includes(vector<token> tokens, vector<string>& include_f = includes_f);
void check_pcllibs(vector<token> tokens);
vector<int> get_functions(vector<token> tokens);
vector<string> get_functions_name(vector<token> tokens);

vector<constant> browse_parameters(vector<token>& tokens);
bool is_int(token t);
bool is_float(token t);
bool is_string(token t);
bool is_char(token t);
bool is_list(token t);
