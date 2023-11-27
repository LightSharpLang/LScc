#include "compilation.h"

void check_externs(vector<token> tokens) {
    /* checking for external functions / variables
    
    an extern is always following this order:
    extern  [constant] ( ) = function
    token 1 [token 2]  3 4

    extern  [constant] = variable
    token 1 [token2]

    
    so we need to identify the token of type Tokentypes::extern and register the following token
    */
    bool next_e = false;
    for (auto t : tokens) {
        if (next_e) {
            if (t.next->type == Tokentypes::parameter_start)
                externs.push_back(constant(t.t, Tokentypes::function, "", Basetype::_any));
            else
                externs.push_back(constant(t.t, Tokentypes::variable, "", Basetype::_any));
            next_e = false;
        }
        if (t.type == Tokentypes::_extern) next_e = true;
    }
}

void check_includes(vector<token> tokens) {
    /* checking for include file

    an include is always following this order:
    include [file]
    token 1 [token 2]

    so we need to identify the token of type Tokentypes::_include and register the following token
    */
    bool next_e = false;
    for (auto t : tokens) {
        if (next_e) {
            includes_f.push_back(t.t);
            next_e = false;
        }
        if (t.type == Tokentypes::_include) next_e = true;
    }
}

void check_pcllibs(vector<token> tokens) {
    /* checking for precompiled libraries

    an include (with) is always following this order:
    with    [path]
    token 1 [token 2]

    so we need to identify the token of value "with" and register the following token
    because we haven't identifyed tokens now we can't use Tokentypes::
    */
    bool next_e = false;
    for (token& t : tokens) {
        if (next_e) {
            pcllibs.push_back(t.t.substr(1, t.t.size() - 2));
            next_e = false;
        }
        if (t.t == "with") next_e = true;
    }
}

constant* compilation::get_registered_constant(string t) {
    for (constant& i : constants) {
        if (i.name == t) return &i;
    }
    return (constant*)((void*)0);
}

size_t compilation::is_registered_constant(string t) {
    for (size_t i = 0; i < constants.size(); i++) {
        if (constants[i].name == t) {
            return i;
        }
    }
    return - 1;

}

vector<constant> compilation::browse_argument(vector<token>& tokens) {
    /*
    This function returns the arguments of a function:
    foo(1, "hello world", i)
    return => vector<> of [1, "hello world", i]
    1 as int
    "hello world" as string
    i as constant (variable)
    */
    vector<constant> arguments;
    int function_start = 0;
    bool op = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::parameter_start) {
            function_start = i;
            break;
        }
    }

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::_operator) {
            op = true;
            break;
        }
    }

    if (!op) {
        for (int i = function_start; i < tokens.size(); i++) {
            if (is_int(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::_int, "", Basetype::_int));
            elif(is_float(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::_float, "", Basetype::_float));
            elif(is_string(tokens[i])) {
                arguments.push_back(constant(tokens[i].t, Tokentypes::_string, "", Basetype::_string));
                ROspaces += 1;
                section_data += "  LC" + to_string(ROspaces) + ": db " + tokens[i].t + ", 0\n";
                arguments[arguments.size() - 1].reg = "LC" + to_string(ROspaces);
            }
            elif(is_char(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::_int, "", Basetype::_int));
            elif(is_list(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::ptr, "", Basetype::_ptr));
            elif(tokens[i].type == Tokentypes::_constant) {
                constant* consta = get_registered_constant(tokens[i].t);
                if (consta == (constant*)((void*)0)) {
                    cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << tokens[i].line << ": constant '" + tokens[i].t + "' used but not defined!" << endl;
                    exit(1);
                }
                arguments.push_back(*consta);
            }
        }

        return arguments;
    }
    else {
        compile_operation(tokens);
        return { constant(REG[0], Tokentypes::operation, REG[0], Basetype::_any)};
    }
}

vector<constant> browse_parameters(vector<token>& tokens) {
    /*
    This function returns the parameters of a function:
    def foo(parameter1, parameter2, ...):
    return => vector<> of [parameter1, parameter2, ...]
    */
    vector<constant> arguments;
    int function_start = 0;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::parameter_start) {
            function_start = i;
            break;
        }
    }
    int i = function_start - 1;
    while (tokens[i++].type != Tokentypes::colon) {
        if (tokens[i].type == Tokentypes::type) arguments.push_back(constant(tokens[i+1].t, Tokentypes::variable, "", getType(tokens[i])));
    }
    return arguments;
}

string compilation::get_reg() {
    string space = "";
    space = "[" + REG[6] + " - " + to_string(this->stack) + "]";
    this->stack += 8; // reserve 1 byte for type identification
    return space;
}

bool is_int(token t) {
    string chars = "x1234567890ABCDEF";
    for (char c : t.t) {
        if (chars.find(c) == string::npos) return false;
    }
    return true;
}
bool is_float(token t) {
    string chars = "1234567890.";
    for (char c : t.t) {
        if (chars.find(c) == string::npos) return false;
    }
    return true;
}
bool is_string(token t) {
    if (t.t[0] == '"') return true;
    return false;
}
bool is_char(token t) {
    if (t.t[0] == '\'') return true;
    return false;
}
bool is_list(token t) {
    if (t.t[0] == '[') return true;
    return false;
}

Tokentypes compilation::detect_constant_type(token tok, vector<token>& line) {
    for (constant c : this->constants) {
        if (c.name == tok.t) {
            return c.type;
        }
    }
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i].t == tok.t) {
            if (i != 0) {
                if (line[i - 1].type == Tokentypes::inv) {
                    if (line[i - 2].type == Tokentypes::condition) {
                        return Tokentypes::condition_exec;
                    }
                } elif(line[i - 1].type == Tokentypes::condition) {
                    if (line[i + 1].type == Tokentypes::condition) {
                        return Tokentypes::condition_def;
                    }
                    else {
                        return Tokentypes::condition_exec;
                    }
                }
                if (line[i - 1].type == Tokentypes::type) {
                    return Tokentypes::variable;
                }
                if (line[i - 1].type == Tokentypes::global) {
                    return Tokentypes::global_variable;
                }
            }
            if (is_int(line[i])) {
                return Tokentypes::_int;
            }
            if (is_float(line[i])) {
                return Tokentypes::_float;
            }
            if (is_string(line[i])) {
                return Tokentypes::_string;
            }
            if (is_char(line[i])) {
                return Tokentypes::_char;
            }
            if (is_list(line[i])) {
                return Tokentypes::ptr;
            }
            if (line[i + 1].type == Tokentypes::parameter_start) return Tokentypes::function;
            warn(Error::typeWarn, line[0].line, stringstream() << "assuming constant type of '" << line[i].t << "' as variable: type not specified");
            return Tokentypes::variable;
        }
    }
    cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << tok.line << ", constant \"" << tok.t << "\" used but not created";
    exit(1);

}

void compilation::register_function(vector<token> tokens) {
    vector<int> function = get_functions(tokens);
    constants.push_back(constant("return", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("nothing", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("int", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("syscall", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("nop", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("label", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("goto", Tokentypes::function, "", Basetype::_any));
    for (int i : function) {
        constants.push_back(constant(tokens[i + 1].t, Tokentypes::function, "", Basetype::_any));
    }
    for (constant e : externs) {
        constants.push_back(e);
    }
}

bool compilation::check_condi(vector<token> line, vector<token> _tokens, int* counter, bool isincondi) {
    for (size_t i = 0; i < line.size(); i++) {
        if (i != 0) {
            if (line[i - 1].type == Tokentypes::inv) {
                if (i != 1)
                    if (line[i - 2].type == Tokentypes::condition) {
                        //line.erase(line.begin(), line.begin() + i + 1);
                        //i += exec_condi(line, _tokens, "", i, &condi);
                        // inline or multiline
                        cout << "Error: cannot do not statements yet!" << endl;
                        exit(1);
                        return true;
                    }
            } 
            elif(line[i - 1].type == Tokentypes::condition) {
                // $1 multiline syntax
                if (line[i + 1].type == Tokentypes::lf) {
                    string cName = line[i].t;
                    *counter = exec_condi(line, _tokens, cName, *counter, true);
                    return false;
                }
                // $[name]$ inline syntax
                if (line[i + 1].type == Tokentypes::condition) {
                    string cName = line[i].t;
                    line.erase(line.begin(), line.begin() + i + 2);
                    exec_condi(line, _tokens, cName, *counter, false);
                    return true;
                }
                // $1 = definition syntax
                elif(line[i + 1].type == Tokentypes::equal) {
                    if (!isincondi) {
                        register_condi(line);
                        return true;
                    }
                    cout << Error::errorTypeError << "Error:" << Error::errorTypeNormal << "can't define a condition insine a condition statement at line " << line[i].line << endl;
                    exit(1);
                    return true;
                }
                else {
                    cout << Error::errorTypeError << "Error:" << Error::errorTypeNormal << "syntax error at line " << line[i].line << endl;
                    exit(1);
                }
            }
        }
    }
    return false;
}

int compilation::exec_condi(vector<token> line, vector<token> _tokens, string name, int _i, bool multiline) {
    // detect condition

    condition currentC;

    for (condition c : conditions) {
        if (c.name == name) {
            currentC = c;
            break;
        }
    }

    if (currentC.name == "") {
        cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << line[0].line << " : cannot find condition named: " << name << endl;
        exit(1);
    }

    // prepare conditon

    L += 1;

    {
        if (currentC.left.reg != "") {
            code << string(ident, ' ') << "mov rax, " << currentC.left.reg << endl;
        }
        else {
            code << string(ident, ' ') << "mov rax, " << currentC.left.name << endl;
        }
    }

    code << string(ident, ' ') << "cmp rax";

    {
        if (currentC.right.reg != "") {
            code << string(ident, ' ') << ", " << currentC.right.reg << endl;
        }
        else {
            code << string(ident, ' ') << ", " << currentC.right.name << endl;
        }
    }

    switch (currentC.op) {
        case (Tokentypes::je):
            code << string(ident, ' ') << "je " << ".L" << L << endl;
            break;
        case (Tokentypes::jz):
            code << string(ident, ' ') << "jz " << ".L" << L << endl;
            break;
        case (Tokentypes::jne):
            code << string(ident, ' ') << "jne " << ".L" << L << endl;
            break;
        case (Tokentypes::jnz):
            code << string(ident, ' ') << "jnz " << ".L" << L << endl;
            break;
        case (Tokentypes::jg):
            code << string(ident, ' ') << "jg " << ".L" << L << endl;
            break;
        case (Tokentypes::jnle):
            code << string(ident, ' ') << "jnle " << ".L" << L << endl;
            break;
        case (Tokentypes::jge):
            code << string(ident, ' ') << "jge " << ".L" << L << endl;
            break;
        case (Tokentypes::jnl):
            code << string(ident, ' ') << "jnl " << ".L" << L << endl;
            break;
        case (Tokentypes::jl):
            code << string(ident, ' ') << "jl " << ".L" << L << endl;
            break;
        case (Tokentypes::jnge):
            code << string(ident, ' ') << "jnge " << ".L" << L << endl;
            break;
        case (Tokentypes::jle):
            code << string(ident, ' ') << "jle " << ".L" << L << endl;
            break;
        case (Tokentypes::jng):
            code << string(ident, ' ') << "jng " << ".L" << L << endl;
            break;
    }

    code << string(ident, ' ') << ".L" << L << ":" << endl;
    ident += 2;

    // code inside satements
    
    if (multiline) {
        bool condi = false;
        int j = _i + 1;
        int linenum = line[0].line + 1; // we wants to skip the first line

        line.erase(line.begin(), line.end());

        for (j; j < _tokens.size() && _tokens[j].type != Tokentypes::condition; j++) {
            if (_tokens[j].type == Tokentypes::lf) {
                if (line[0].line >= linenum) {
                    line.push_back(_tokens[j]);
                    code << "; line " << line[0].line << endl;
                    if (line[0].type == Tokentypes::condition && line[1].type == Tokentypes::lf) {
                        j++;
                        break;
                    }
                    condi = check_condi(line, _tokens, &j, true);
                    if (condi == false) {
                        interpret_and_compile_var(line);
                        interpret_and_compile(line);
                    }
                    line.erase(line.begin(), line.end());
                }
                else {
                    line.erase(line.begin(), line.end());
                }
            }
            else line.push_back(_tokens[j]);
        }
        ident -= 2;
        return j;
    }
    else {
        bool condi = false;
        int i = 0;
        condi = check_condi(line, _tokens, &i, true);
        if (!condi) {
            interpret_and_compile_var(line);
            interpret_and_compile(line);
        }
        line.erase(line.begin(), line.end());
    }

    ident -= 2;

    return 1;
}

void compilation::register_condi(vector<token> line) {
    condition c;
    // get condition's name
    c.name = line[1].t; // the name is always after the $ which must be the first token
    // get left operand
    // in order to do so we need to know the position of the '=' sign which is right after the name
    Tokentypes optype = detect_constant_type(line[3], line);
    if (optype == Tokentypes::variable || optype == Tokentypes::global_variable || optype == Tokentypes::function) {
        if (is_registered_constant(line[3].t) != -1) {
            c.left = *get_registered_constant(line[3].t);

        }
        else {
            cout << Error::errorTypeError << "Error at line " << line[0].line << " constant \"" << line[3].t << "\" undefined!" << Error::errorTypeNormal << endl;;
            exit(1);
        }
    }
    else {
        c.left = constant(line[3].t, optype, "", Basetype::_any);
    }
    // get operator
    c.op = line[4].type;
    // get right operand
    optype = detect_constant_type(line[5], line);
    if (optype == Tokentypes::variable || optype == Tokentypes::global_variable || optype == Tokentypes::function) {
        if (is_registered_constant(line[5].t) != -1) {
            c.right = *get_registered_constant(line[5].t);

        }
        else {
            cout << Error::errorTypeError << "Error at line " << line[0].line << " constant \"" << line[5].t << "\" undefined!" << Error::errorTypeNormal << endl;;
            exit(1);
        }
    }
    else {
        c.right = constant(line[5].t, optype, "", Basetype::_any);
    }
    // register
    conditions.push_back(c);
}

string compilation::interpret_and_compile_var(vector<token>& _tokens) {
    for (size_t i = 0; i < _tokens.size(); i++) {
        if ((_tokens[i].type == Tokentypes::equal || _tokens[i].type == Tokentypes::_operator) && i > 0) {
            if (_tokens[i].type == Tokentypes::equal) {
                size_t consta = is_registered_constant(_tokens[i - 1].t);
                string value = browse_value(_tokens);
                if (consta == -1) {
                    if (detect_constant_type(_tokens[i - 1], _tokens) == Tokentypes::global_variable) {
                        value = browse_value(_tokens, true);
                        Basetype type = Basetype::_any;
                        if (_tokens[0].type == Tokentypes::type) {
                            type = getType(_tokens[0]);
                        }
                        spaces.push_back(constant(_tokens[i - 1].t, Tokentypes::global_variable, _tokens[i - 1].t, type));
                        section_data += "  " + _tokens[i - 1].t + ": dq " + value;
                        constants.push_back(constant(_tokens[i - 1].t, Tokentypes::global_variable, _tokens[i - 1].t, type));
                    }
                    else {
                        string reg = get_reg();
                        Basetype type = Basetype::_any;
                        if (_tokens[0].type == Tokentypes::type) {
                            type = getType(_tokens[0]);
                        }
                        constants.push_back(constant(_tokens[i - 1].t, Tokentypes::variable, reg, type));
                        code << string(ident, ' ') << "mov qword " << reg << ", " << value << endl;
                    }
                }
                elif(constants[consta].type == Tokentypes::variable) {
                    Basetype type = constants[consta].lastUse;
                    if (_tokens[0].type == Tokentypes::type) {
                        type = getType(_tokens[0]);
                    }
                    if (type != constants[consta].lastUse) {
                        warn(Error::typeWarn, _tokens[0].line, stringstream() << "changing type of variable \"" << constants[consta].name << "\" from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!");
                        constants[consta].lastUse = type;
                    }
                    if (value != constants[consta].reg) {
                        if (type == Basetype::_float) {
                            code << string(ident, ' ') << "mov rax, " << value << endl;
                            code << string(ident, ' ') << "pxor xmm0, xmm0" << endl;
                            code << string(ident, ' ') << "cvtsi2sd xmm0, rax" << endl;
                            code << string(ident, ' ') << "mov qword  " << constants[consta].reg << ", xmm0" << endl;
                        }
                        else {
                            code << string(ident, ' ') << "mov qword  " << constants[consta].reg << ", " << value << endl;
                        }
                    }
                }
                elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                    Basetype type = Basetype::_any;
                    if (_tokens[0].type == Tokentypes::type) {
                        type = getType(_tokens[0]);
                    }
                    if (type != constants[consta].lastUse) {
                        warn(Error::typeWarn, _tokens[0].line, stringstream() << "changing type of variable \"" << constants[consta].name << "\" from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!");
                        constants[consta].lastUse = type;
                    }
                    if (in(constants[consta].name, REG)) {
                        code << string(ident, ' ') << "mov " << REG[0] << ", " << value << endl;
                        code << string(ident, ' ') << "mov " << constants[consta].name << ", " << REG[0] << endl;
                    }
                    else {
                        code << string(ident, ' ') << "mov " << REG[0] << ", " << value << endl;
                        code << string(ident, ' ') << "mov " << constants[consta].reg << ", " << REG[0] << endl;
                    }

                }
                break;
            }
            else {
                size_t consta = is_registered_constant(_tokens[i - 1].t);
                if (consta == -1) {
                    consta = is_registered_constant(_tokens[i + 1].t);
                    if (consta == -1) {
                        cout << Error::errorTypeError << "Error at line: " << Error::errorTypeNormal << _tokens[0].line << " cannot use undefined variable '" << _tokens[i - 1].t << "'!" << endl;
                        exit(1);
                    }
                    elif(constants[consta].type == Tokentypes::variable) {
                        _tokens.insert(_tokens.begin(), { token(_tokens[i + 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                        string value = browse_value(_tokens);
                        code << string(ident, ' ') << "mov qword  " << constants[consta].reg << ", " << value << endl;
                    }
                    elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                        _tokens.insert(_tokens.begin(), { token(_tokens[i + 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                        string value = browse_value(_tokens);
                        code << string(ident, ' ') << "mov " << REG[0] << ", " << value << endl;
                        code << string(ident, ' ') << "mov " << constants[consta].reg << ", " << REG[0] << endl;
                    }
                }
                elif(constants[consta].type == Tokentypes::variable) {
                    _tokens.insert(_tokens.begin(), { token(_tokens[i - 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                    string value = browse_value(_tokens);
                    code << string(ident, ' ') << "mov qword  " << constants[consta].reg << ", " << value << endl;
                }
                elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                    _tokens.insert(_tokens.begin(), { token(_tokens[i - 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                    string value = browse_value(_tokens);
                    code << string(ident, ' ') << "mov " << REG[0] << ", " << value << endl;
                    code << string(ident, ' ') << "mov " << constants[consta].reg << ", " << REG[0] << endl;
                }
                break;
            }
        }
    }
    return code.str();
}

string compilation::interpret_and_compile(vector<token>& _tokens) {
    string fname = "";

    for (size_t i = 0; i < _tokens.size(); i++) {
        if (_tokens[i].type == Tokentypes::_constant || _tokens[i].t == "int") {
            if (_tokens[i].next->type == Tokentypes::parameter_start) {
                size_t consta = is_registered_constant(_tokens[i].t);
                if (consta != -1) {
                    if (constants[consta].type == Tokentypes::function || constants[consta].type == Tokentypes::_extern) {
                        if (_tokens[i + 1].type == Tokentypes::parameter_start) fname = constants[consta].name;
                        break;
                    }
                    else {
                        if (detect_constant_type(_tokens[i], _tokens) == Tokentypes::function) {
                            cout << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": function " << _tokens[i].t << " called but not created";
                            exit(1);
                        }
                    }
                }
                else {
                    cout << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << ": Cannot call non existant function \"" << _tokens[i].t << "\" !" << endl;
                    exit(1);
                }
            }
        }
    }
    if (fname != "") {
        vector<constant> parameters = browse_argument(_tokens);
        if (fname == "return") {
            if (parameters.size() > 0) {
                if (parameters[0].reg != "") {
                    code << string(ident, ' ') << "mov " << REG[7] << ", " << REG[6] << endl;
                    code << string(ident, ' ') << "pop " << REG[6] << endl;
                    code << string(ident, ' ') << "mov " << REG[0] << ", " << parameters[0].reg << endl;
                    code << string(ident, ' ') << "ret" << endl;
                }
                else {
                    code << string(ident, ' ') << "mov " << REG[7] << ", " << REG[6] << endl;
                    code << string(ident, ' ') << "pop " << REG[6] << endl;
                    code << string(ident, ' ') << "mov " << REG[0] << ", " << parameters[0].name << endl;
                    code << string(ident, ' ') << "ret" << endl;
                }
            }
            else {
                code << string(ident, ' ') << "ret" << endl;
            }
        }
        elif(fname == "nop") {
            code << string(ident, ' ') << "nop" << endl;
        }
        elif(fname == "int") {
            if (parameters[0].reg != "") code << string(ident, ' ') << "int " << parameters[0].reg << endl;
            else code << string(ident, ' ') << "int " << parameters[0].name << endl;
        }
        elif(fname == "syscall") {
            code << string(ident, ' ') << "syscall" << endl;
        }
        elif(fname == "label") {
            code << "." << parameters[0].name << ":" << endl;
            this->labels.push_back(parameters[0].name);
        }
        elif(fname == "goto") {
            if (in(parameters[0].name, this->labels)) {
                code << string(ident, ' ') << "jmp ." << parameters[0].name << endl;
            }
            else {
                cout << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[0].line << ": label \"" << parameters[0].name << "\" does not exist!" << endl;
                exit(1);
            }
        }
        else {
            reverse(parameters.begin(), parameters.end());
            size_t p_num = -1;
            for (constant i : parameters) {
                p_num++;
                if (p_num < argument_order.size()) {
                    if (i.reg.rfind("LC", 0) == 0) code << string(ident, ' ') << "lea " << REG[argument_order[p_num]] << ", [rel " << i.reg << "]\n";
                    elif(i.reg != "") code << string(ident, ' ') << "mov " << REG[argument_order[p_num]] << ", " << i.reg << "\n";
                    else code << string(ident, ' ') << "mov " << REG[argument_order[p_num]] << ", " << i.name << "\n";
                }
                else {
                    if (i.reg != "") code << string(ident, ' ') << "push " << i.reg << "\n";
                    else code << string(ident, ' ') << "push " << i.name << "\n";
                }
            }
            code << string(ident, ' ') << "xor " << REG[0] << ", " << REG[0] << endl;
            code << string(ident, ' ') << "call " << fname << endl;
        }
    }

    return code.str();
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    bool replaced = false;
    string substr;

    size_t i = 0;
    while (str[i++] != 0) {
        substr = str.substr(i, str.length());
        if (substr.find(from) == 0) {
            string checkstr = std::string(1, str[i - 1]);

            if (alnum(checkstr)) continue;
            checkstr = std::string(1, str[i + from.length()]);
            if (alnum(checkstr)) continue;

            str.erase(i, from.length());
            str.insert(i, to);
            i += to.length();

            replaced = true;
        }
    }
    return replaced;
}

void compilation::compile_operation(vector<token>& _tokens) {
    // process left to right - parenthesis ignored

    size_t eq = 0;
    bool op = false;
    bool bone = true;
    for (size_t i = 0; i < _tokens.size(); i++) {
        if (_tokens[i].type == Tokentypes::equal) {
            eq = i;
            break;
        }
    }

    constant one("", Tokentypes::null, "", Basetype::_any);
    constant two("", Tokentypes::null, "", Basetype::_any);

    for (size_t i = eq; i < _tokens.size(); i++) {
        if (is_int(_tokens[i])) {
            if (bone) {
                one = constant(_tokens[i].t, Tokentypes::_int, "", Basetype::_int);
                bone = false;
            }
            else {
                two = constant(_tokens[i].t, Tokentypes::_int, "", Basetype::_int);
                bone = true;
            }
        }
        elif(is_float(_tokens[i])) {
            if (bone) {
                one = constant(_tokens[i].t, Tokentypes::_float, "", Basetype::_float);
                bone = false;
            }
            else {
                two = constant(_tokens[i].t, Tokentypes::_float, "", Basetype::_float);
                bone = true;
            }
        }
        elif(is_string(_tokens[i])) {
            ROspaces += 1;
            section_data += "  LC" + to_string(ROspaces) + ": db" + _tokens[i].t + ", 0\n";
            if (bone) {
                one = constant("LC" + to_string(ROspaces), Tokentypes::_string, "", Basetype::_string);
                bone = false;
            }
            else {
                two = constant("LC" + to_string(ROspaces), Tokentypes::_string, "", Basetype::_string);
                bone = true;
            }
        }
        elif(is_char(_tokens[i])) {
            if (bone) {
                one = constant(_tokens[i].t, Tokentypes::_char, "", Basetype::_any);
                bone = false;
            }
            else {
                two = constant(_tokens[i].t, Tokentypes::_char, "", Basetype::_any);
                bone = true;
            }
        }
        elif(is_list(_tokens[i])) {
            if (bone) {
                one = constant(_tokens[i].t, Tokentypes::ptr, "", Basetype::_ptr);
                bone = false;
            }
            else {
                two = constant(_tokens[i].t, Tokentypes::ptr, "", Basetype::_ptr);
                bone = true;
            }
        } // not accurate
        elif(_tokens[i].type == Tokentypes::_constant) {
            size_t consta = is_registered_constant(_tokens[i].t);
            if (consta == -1)
            {
                cout << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": constant " << _tokens[i].t << " used but not defined";
                exit(1);
            }
            string value;
            if (constants[consta].reg != "") {
                if (bone) {
                    one = constants[consta];
                    one.name = one.reg;
                    bone = false;
                }
                else {
                    two = constants[consta];
                    two.name = "qword " + two.reg;
                    bone = true;
                }
            }
            else {
                if (bone) {
                    one = constants[consta];
                    bone = false;
                }
                else {
                    two = constants[consta];
                    bone = true;
                }
            }
        }

        if (op) {
            coperator ope;
            for (coperator o : operators) {
                if (o.name == _tokens[i - 1].t) {
                    if (one.lastUse == o.parameters[0].lastUse ||
                        one.lastUse == Basetype::_any ||
                        o.parameters[0].lastUse == Basetype::_any) {

                        if (two.lastUse == o.parameters[1].lastUse ||
                            two.lastUse == Basetype::_any ||
                            o.parameters[0].lastUse == Basetype::_any) {

                            ope = o;
                            break; 
                        }
                    }
                }
            }

            if (ope.name == "") {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " could not find any operators for types " << fromType(one.lastUse) << " and " << fromType(two.lastUse) << "!" << endl;
                exit(1);
            }

            if (ope.parameters.size() < 2 || ope.parameters.size() > 2) {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameters lenght not equal to two";
                exit(1);
            }
            
            if (!replace(ope.code, ope.parameters[0].name, two.name)) {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameter 0 cannot be found";
                exit(1);
            }

            if (!replace(ope.code, ope.parameters[1].name, one.name)) {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameter 1 cannot be found";
                exit(1);
            }

            code << ope.fcode(ident);
            return;

        }

        if (_tokens[i].type == Tokentypes::_operator) {
            op = true;
        }
        else {
            op = false;
        }
    }
}

string compilation::browse_value(vector<token>& _tokens, bool is_global) {
    size_t eq = 0;
    bool op = false;
    for (size_t i = 0; i < _tokens.size(); i++) {
        if (_tokens[i].type == Tokentypes::equal) {
            eq = i;
            break;
        }
    }
    // check for operators

    for (size_t i = 0; i < _tokens.size(); i++) {
        if (_tokens[i].type == Tokentypes::_operator) {
            op = true;
            break;
        }
    }

    if (!op) {
        for (int i = eq; i < _tokens.size(); i++) {
            if (is_int(_tokens[i])) return _tokens[i].t;
            elif(is_float(_tokens[i])) return _tokens[i].t;
            elif(is_string(_tokens[i])) {
                string value;
                if (is_global == false) {
                    ROspaces += 1;
                    section_data += "  LC" + to_string(ROspaces) + ": db" + _tokens[i].t + ", 0\n";
                    value = "LC" + to_string(ROspaces);
                }
                else value = _tokens[i].t;
                return value;
            }
            elif(is_char(_tokens[i])) return _tokens[i].t;
            elif(is_list(_tokens[i])) return "ptr";

            elif(_tokens[i].type == Tokentypes::_constant) {
                size_t consta = is_registered_constant(_tokens[i].t);
                if (consta == -1)
                {
                    cout << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": constant " << _tokens[i].t << " used but not defined";
                    exit(1);
                }
                string value;
                if (constants[consta].type == Tokentypes::function) {
                    value = REG[0];
                }
                else {
                    if (constants[consta].reg != "") value = constants[consta].reg;
                    else value = constants[consta].name;
                }
                return value;

            }
        }
    }
    else {
        compile_operation(_tokens);
        return REG[0];
    }
}

string compilation::compile_function(int function_start, vector<token>& _tokens) {
    string name = "";
    vector<token> tokens;
    bool is_first_line = true;

    // clear local variables of previous functions
    constants.clear();
    conditions.clear();
    this->stack = 8;

    // getting the function's tokens
    for (int i = function_start; i < _tokens.size(); i++) {
        if (_tokens[i].type != Tokentypes::end) {
            tokens.push_back(_tokens[i]);
        }
        else {
            break;
        }
    }

    constants.insert(constants.end(), spaces.begin(), spaces.end());

    bool parenthesis = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::parameter_start) parenthesis = true;
        if (tokens[i].type == Tokentypes::parameter_end) {
            parenthesis = false;
            break;
        }
        if (tokens[i].type == Tokentypes::_constant && tokens[i - 1].type == Tokentypes::definition) {
            name = tokens[i].t;
            code.str("");
            code << "GLOBAL " << name << "\n" << name << ":\n  push " << REG[6] << "\n  mov " << REG[6] << ", " << REG[7] << "\n";
        }
        if (tokens[i].type == Tokentypes::_constant && parenthesis) {
            if (tokens[i - 1].type == Tokentypes::type) {
                code << string(ident, ' ') << "mov qword [" << REG[6] << " - " << to_string(this->stack) << "], " << REG[argument_order[i]] << "\n";
                constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", getType(tokens[i - 1])));
                this->stack += 8;
            }
            else {
                code << string(ident, ' ') << "mov qword [" << REG[6] << " - " << to_string(this->stack) << "], " << REG[argument_order[i]] << "\n";
                constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", Basetype::_any));
                this->stack += 8;
            }
        }
    }

    vector<token> line;

    register_function(_tokens);

    // iterate through function's tokens
    bool condi = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::lf) {
            if (!is_first_line) {
                line.push_back(tokens[i]);
                condi = check_condi(line, tokens, &i, false);
                if (condi == false) {
                    interpret_and_compile(line);
                    interpret_and_compile_var(line);
                }
                line.erase(line.begin(), line.end());
            }
            else {
                is_first_line = false;
            }
        }
        elif(!is_first_line) line.push_back(tokens[i]);
    }
    return code.str();
}

vector<int> get_functions(vector<token> tokens) {
    vector<int> functions;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::definition && tokens[i + 1].type == Tokentypes::_constant) {
            functions.push_back(i);
        }
    }
    return functions;
}

vector<string> get_functions_name(vector<token> tokens) {
    vector<string> functions;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::definition && tokens[i + 1].type == Tokentypes::_constant) {
            functions.push_back(tokens[i+1].t);
        }
    }
    return functions;
}

bool precompilation::is_operator(vector<token> _tokens, size_t index) {
    if (_tokens[index + 1].type == Tokentypes::_operator) return true;
    return false;
}
bool precompilation::is_child(vector<token> _tokens, size_t index) {
    if (_tokens[index + 1].type == Tokentypes::_operator) return true;
    return false;
}

string precompilation::precompile_lib(vector<token>& _tokens) {
    size_t index = 0;
    bool start = false;
    bool bop = false;
    bool bchild = false;
    bool bfunc = false;
    vector<constant> arguments;
    coperator op;
    stringstream code;
    token last("\n"); last.type = Tokentypes::lf;

    for (token t : _tokens) {
        if (t.type == Tokentypes::type) {
            if (is_operator(_tokens, index)) {

                op.type = Basetype::_any;

                if (t.type == Tokentypes::type) {
                    op.type = getType(t);
                }

                // getting operator's type

                // unify operator name (x tokens not only one)
                // searching for the open parenthesis token
                int parenthesis = index + 1;
                while (_tokens[parenthesis++ + 1].type != Tokentypes::parameter_start) 
                    op.name += _tokens[parenthesis].t;
 
                vector<token> toks;
                toks.insert(toks.end(), _tokens.begin() + index, _tokens.end());

                arguments = browse_parameters(toks);

                for (constant p : arguments) {
                    op.parameters.push_back(p);
                }

                bop = true;
            }
            elif(is_child(_tokens, index)) {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " childs not implemented" << endl;
                //exit(1);
            }
            elif (!bop && !bchild){
                cout << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[index].line << " : precompiled functions not implemented" << endl;
                // function
            }
        }
        elif(t.type == Tokentypes::colon) {
            code.str("");
            start = true;
        }
        elif(t.type == Tokentypes::end) {
            start = false;

            if (bop) {
                op.code = code.str();
                operatorslib.push_back(op);
                op.name = "";
                op.parameters.clear();
                op.code = "";
            }
            elif(bchild) {
                cout << Error::errorTypeWarn << "warning:" << Error::errorTypeNormal << " childs not implemented!" << __FILE__ << __FUNCTION__ << __LINE__;
            }
            else {
                cout << Error::errorTypeWarn << "warning:" << Error::errorTypeNormal << " precompiled_functions not implemented!" << __FILE__ << __FUNCTION__ << __LINE__;
            }

            bool bop = false;
            bool bchild = false;
            bool bfunc = false;
        }
        elif (start) {
            if (last.type == Tokentypes::lf) code << " " << t.t;
            elif (t.type != Tokentypes::separator) code << " " << t.t;
            else code << t.t;
        }
        index++;
        last = t;
    }
    return "";
}