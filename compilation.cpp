#include "compilation.h"

compilation compile_file(string file, string& section_text, compilation c) {
    vector<token> tokens = tokenize(file);
    precompilation clib;
    vector<token> tokenlibs;

    if (!filesystem::exists(filesystem::path(file))) {
        cerr << Error::errorTypeError << "Error: file \"" << file << "\" not found!\n" << Error::errorTypeNormal;
        exit(1);
    }

    if (Args[LsccArg::info] == "1") 
        cout << Error::errorTypeInfo << "Info: Starting tokenizing file: " + file << "at: " << std::time(nullptr) << '\n' << Error::errorTypeNormal;

    check_pcllibs(tokens);

    if (pcllibs.size() > 0) {
        for (size_t i = 0; i < pcllibs.size(); i++) {
            tokenlibs.clear();
            tokenlibs = tokenize(pcllibs[i]);
            fuse_symbols(&tokenlibs);
            identify_tokens(&tokenlibs, true);
            clib.precompile_lib(tokenlibs);
        }

        if (clib.operatorslib.size() > 0) {
            for (size_t i = 0; i < clib.operatorslib.size(); i++) {
                operators.push_back(clib.operatorslib[i]);
            }
        }

        if (clib.childslib.size() > 0) {
            for (size_t i = 0; i < clib.childslib.size(); i++) {
                childs.push_back(clib.childslib[i]);
            }
        }
    }

    vector<string> subincludes;

    check_includes(tokens, subincludes);
    check_redef(tokens);

    // compiling includes

    for (string& e : subincludes) {
        if (e.size() <= 1) continue;
        string ne = e.substr(1, e.size() - 2);
        filesystem::path fileOption1 = (filesystem::current_path() / filesystem::path(ne));
        filesystem::path fileOption2 = (WorkingDirectory / filesystem::path(ne));
        vector<token> i_tokens;
        if (filesystem::exists(fileOption1)) {
            compilation nc = compile_file(fileOption1.string(), section_text, c);
            c.labels.insert(c.labels.end(), nc.labels.begin(), nc.labels.end());
            for (auto& i : nc.constants) c.add_external_constant(i);
            c.code << nc.code.str();
            c.L = nc.L;
        }
        else {
            compilation nc = compile_file(fileOption2.string(), section_text, c);
            c.labels.insert(c.labels.end(), nc.labels.begin(), nc.labels.end());
            for (auto& i : nc.constants) c.add_external_constant(i);
            c.code << nc.code.str();
            c.L = nc.L;
        }
    }

    if (Args[LsccArg::info] == "1")
        cout << Error::errorTypeInfo << "Info: Finished tokenizing file: " + file << "at: " << std::time(nullptr) << '\n' << Error::errorTypeNormal;

    currentFile = file;
    fuse_symbols(&tokens);
    identify_tokens(&tokens);
    fuse_structs(&tokens);
    check_externs(tokens);

    vector<int> functions = get_functions(tokens);
    vector<int> structs = get_structures(tokens);
    sort(functions.begin(), functions.end());
    functions.erase(unique(functions.begin(), functions.end()), functions.end());

    c.__tokens = tokens;

    if (Args[LsccArg::info] == "1")
        cout << Error::errorTypeInfo << "Info: Starting compiling file: " + file << "at: " << std::time(nullptr) << '\n' << Error::errorTypeNormal;

    for (int f : functions) {
        preproc.processFile(f, tokens);
    }

    for (int f : structs) {
        register_structures(f, tokens);
    }

    for (int f : functions) {
        section_text += c.compile_function(f, tokens);
    }

    if (Args[LsccArg::info] == "1")
        cout << Error::errorTypeInfo << "Info: Finished ompiling file: " + file << "at: " << std::time(nullptr) << '\n' << Error::errorTypeNormal;

    return c;
}

void check_externs(vector<token> tokens) {
    /* checking for external functions / variables

    an extern is always following this order:
    extern  [constant] ( ) = function
    token 1 [token 2]  3 4

    extern  [constant] = variable
    token 1 [token2]


    so we need to identify the token of type Tokentypes::extern and register the following token
    */
    bool next_e = false, rel = false;
    for (auto t : tokens) {
        if (t.type == Tokentypes::rel) {
            rel = true;
        }
        else if (next_e) {
            if (t.next->type == Tokentypes::parameter_start)
                if (rel) externs.push_back(constant(t.t, Tokentypes::_extern, "", Basetype::_any));
                else {
                    externs.push_back(constant(t.t, Tokentypes::function, "", Basetype::_any));
                    rel = false;
                }
            else
                externs.push_back(constant(t.t, Tokentypes::global_variable, "", Basetype::_any));
            next_e = false;
        }
        if (t.type == Tokentypes::_extern) next_e = true;
    }
}

void check_includes(vector<token> tokens, vector<string>& include_f) {
    /* checking for include file

    an include is always following this order:
    include [file]
    token 1 [token 2]

    so we need to identify the token "include" and register the following token
    */
    bool next_e = false;
    for (auto t : tokens) {
        if (next_e) {
            include_f.push_back(t.t);
            next_e = false;
        }
        if (t.t == "include") next_e = true;
    }
}

void check_redef(vector<token> tokens) {
    /* checking for redefinition

    a redefinition is always following this order:
    redef   [token1]    [token2]
    token 1 [token 2]   [token3]

    so we need to identify the token "redef" and register the following tokens
    */
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].t == "redef") {
            Tokentypes ntype = tokens_dict[tokens[i + 2].t];
            tokens_dict.insert({ tokens[i + 1].t, ntype });
            cout << "token redefined:" << tokens[i + 1].t << " " << (int)ntype << endl;
        }
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

vector<constant> compilation::browse_argument(vector<token>& tokens, bool ignoreConstantError) {
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

    // check for operators

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::_operator) {
            op = true;
            break;
        }
    }

    if (!op) {
        for (int i = function_start; i < tokens.size(); i++) {
            if (is_int(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::_int, "", Basetype::_int));
            elif(is_float(tokens[i])) {
                ROspaces += 1;
                section_data += "  LC" + to_string(ROspaces) + ": dq " + tokens[i].t + '\n';
                arguments.push_back(constant(tokens[i].t, Tokentypes::_float, "LC" + to_string(ROspaces), Basetype::_float));
            }
            elif(is_string(tokens[i])) {
                arguments.push_back(constant(tokens[i].t, Tokentypes::_string, "", Basetype::_string));
                ROspaces += 1;
                string processedStr = formatString(tokens[i].t);
                section_data += "  LC" + to_string(ROspaces) + ": db " + processedStr + '\n';
                arguments[arguments.size() - 1].reg = "LC" + to_string(ROspaces);
            }
            elif(is_char(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::_int, "", Basetype::_int));
            elif(is_list(tokens[i])) arguments.push_back(constant(tokens[i].t, Tokentypes::ptr, "", Basetype::_ptr));
            elif(tokens[i].type == Tokentypes::parameter_end) return arguments;
            elif(tokens[i].type == Tokentypes::_constant) {
                constant* consta = get_registered_constant(tokens[i].t);
                if (ignoreConstantError == false) {
                    if (consta == (constant*)((void*)0)) {
                        cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << tokens[i].line << ": constant '" + tokens[i].t + "' used but not defined!" << endl;
                        std::exit(1);
                    }
                    else {
                        if (tokens[i + 1].type == Tokentypes::parameter_start) {
                            // crop and interpret again
                            vector<token> subTokens = tokens;
                            for (size_t j = 0; j < subTokens.size(); j++) {
                                if (subTokens[j].type == Tokentypes::parameter_start) {
                                    subTokens.erase(subTokens.begin(), subTokens.begin() + j + 1);
                                    break;
                                }
                            }
                            for (size_t j = 1; j < subTokens.size(); j++) {
                                if (subTokens[subTokens.size() - j].type == Tokentypes::parameter_end) {
                                    subTokens.erase(subTokens.end() - j, subTokens.end());
                                    i += subTokens.size();
                                    break;
                                }
                            }
                            subTokens.push_back(token("\n", subTokens[0].line, Tokentypes::lf));
                            interpret_and_compile(subTokens);
                            arguments.push_back(constant(REG[0], Tokentypes::global_variable, "", Basetype::_any));
                        }
                        else {
                            arguments.push_back(*consta);
                        }
                    }
                }
                else {
                    if (consta == (constant*)((void*)0)) {
                        arguments.push_back(constant(tokens[i].t, Tokentypes::_constant, "", Basetype::_ptr));
                    }
                    else {
                        if (tokens[i + 1].type == Tokentypes::parameter_start) {
                            // crop and interpret again
                            vector<token> subTokens = tokens;
                            for (size_t j = 0; j < subTokens.size(); j++) {
                                if (subTokens[j].type == Tokentypes::parameter_start) {
                                    subTokens.erase(subTokens.begin(), subTokens.begin() + j + 1);
                                    break;
                                }
                            }
                            for (size_t j = 1; j < subTokens.size(); j++) {
                                if (subTokens[subTokens.size() - j].type == Tokentypes::parameter_end) {
                                    subTokens.erase(subTokens.end() - j, subTokens.end());
                                    break;
                                }
                            }

                            subTokens.push_back(token("\n", subTokens[0].line, Tokentypes::lf));
                            interpret_and_compile(subTokens);
                            arguments.push_back(constant(REG[0], Tokentypes::global_variable, "", Basetype::_any));
                        }
                        else {
                            arguments.push_back(*consta);
                        }
                    }
                }
            }
            elif(tokens[i].type == Tokentypes::type) {
                arguments.push_back(constant(tokens[i].t, Tokentypes::_constant, "", Basetype::_none));
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

void register_structures(int i, vector<token> tokens) {
    structure nstruct;
    
    nstruct.name = tokens[i + 1].t;

    for (i + 1; tokens[i].type != Tokentypes::struct_end; i++) {
        if (tokens[i].type == Tokentypes::type) {
            Basetype t = getType(tokens[i].t);
            nstruct.vars.push_back({ tokens[i + 1].t, t });
            nstruct.size += 8;
        }
    }

    structs.push_back(nstruct);
    type_dict.insert(type_dict.end(), { nstruct.name, Basetype::customStructure });
}

string compilation::get_reg() {
    /*
    This function gives a new reserved space in stack for local variables
    */
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

compilation::compilation(compilation& c) {
    // not a replacement but an addition
    this->labels.insert(this->labels.end(), c.labels.begin(), c.labels.end());
    this->constants.insert(this->constants.end(), c.constants.begin(), c.constants.end());
    this->code << c.code.str();
}

compilation compilation::operator =(const compilation& nc) {
    // not a replacement but an addition
    compilation c;
    c.labels.insert(c.labels.end(), nc.labels.begin(), nc.labels.end());
    c.constants.insert(c.constants.end(), nc.constants.begin(), nc.constants.end());
    c.code << nc.code.str();
    c.L = nc.L;
    return c;
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
            return Tokentypes::variable;
        }
    }
    cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << tok.line << ", constant \"" << tok.t << "\" used but not created in file \"" << line[0].file << "\"\n";
    std::exit(1);

}

void compilation::register_function(vector<token> tokens) {
    vector<int> function = get_functions(tokens);
    constants.push_back(constant("return", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("int", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("fix", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("unsign", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("sign", Tokentypes::function, "", Basetype::_any));
    constants.push_back(constant("is", Tokentypes::function, "", Basetype::_any));
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

void compilation::swap() {
    string temp = code.str();
    code.str("");
    code << conditionCode.str().c_str();
    conditionCode.str("");
    conditionCode << temp.c_str();
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

bool compilation::check_condi(vector<token> line, vector<token> _tokens, int* counter, bool isincondi) {
    for (size_t i = 0; i < line.size(); i++) {
        if (i != 0) {
            if(line[i - 1].type == Tokentypes::inv) {
                // $1 multiline syntax
                if (line[i + 1].type == Tokentypes::lf) {
                    string cName = line[i].t;
                    *counter = exec_condi(line, _tokens, cName, *counter, true, true);
                    return false;
                }
                // $[name]$ inline syntax
                if (line[i + 1].type == Tokentypes::condition) {
                    string cName = line[i].t;
                    line.erase(line.begin(), line.begin() + i + 2);
                    exec_condi(line, _tokens, cName, *counter, false, true);
                    return true;
                }
                else {
                    cerr << Error::errorTypeError << "Error:" << Error::errorTypeNormal << "syntax error at line " << line[i].line << endl;
                    std::exit(1);
                }
            }
            elif(line[i - 1].type == Tokentypes::condition) {
                // $1 multiline syntax
                if (line[i + 1].type == Tokentypes::lf) {
                    string cName = line[i].t;
                    *counter = exec_condi(line, _tokens, cName, *counter, true, false);
                    return false;
                }
                // $[name]$ inline syntax
                if (line[i + 1].type == Tokentypes::condition) {
                    string cName = line[i].t;
                    line.erase(line.begin(), line.begin() + i + 2);
                    exec_condi(line, _tokens, cName, *counter, false, false);
                    return true;
                }
                // $1 = definition syntax
                elif(line[i + 1].type == Tokentypes::equal) {
                    if (!isincondi) {
                        register_condi(line);
                        return true;
                    }
                    cerr << Error::errorTypeError << "Error:" << Error::errorTypeNormal << "can't define a condition insine a condition statement at line " << line[i].line << endl;
                    std::exit(1);
                    return true;
                }
                else {
                    cerr << Error::errorTypeError << "Error:" << Error::errorTypeNormal << "syntax error at line " << line[i].line << endl;
                    std::exit(1);
                }
            }
        }
    }
    return false;
}

int compilation::exec_condi(vector<token> line, vector<token> _tokens, string name, int _i, bool multiline, bool inverted) {
    // detect condition

    condition currentC;
    bool execute = true;
    bool isDynamic = false;

    for (condition c : conditions) {
        if (c.name == name) {
            currentC = c;
            break;
        }
    }

    if (currentC.name == "") {
        cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << line[0].line << " : cannot find condition named: " << name << endl;
        std::exit(1);
    }

    //check is the condition is evaluated
    if (currentC.op == Tokentypes::is) {
        if (currentC.right.name == "UNSIGNED" && currentC.left.isUnsigned) {
            isDynamic = true;
            goto condi;
        }
        if (currentC.right.name == "SIGNED" && !currentC.left.isUnsigned) {
            isDynamic = true;
            goto condi;
        }
        if (fromType(currentC.left.lastUse) == currentC.right.name) {
            isDynamic = true;
            goto condi;
        }
        else {
            execute = false;
            isDynamic = true;
        }
    }

    // prepare conditon

    if (execute) {
        L += 1;

        {
            if (currentC.left.reg != "") {
                code << "  mov " << REG[2] << ", qword" << currentC.left.reg << endl;
            }
            else {
                code << "  mov " << REG[2] << ", " << currentC.left.name << endl;
            }
        }
        {
            if (currentC.right.reg != "") {
                code << "  mov " << REG[1] << ", qword " << currentC.right.reg << endl;
            }
            else {
                code << "  mov " << REG[1] << ", " << currentC.right.name << endl;
            }
        }
        code << "  cmp " << REG[2] << ", " << REG[1] << endl;

        if (!inverted) {
            switch (currentC.op) {
            case (Tokentypes::je):
                code << "  je " << ".L" << L << endl;
                break;
            case (Tokentypes::jz):
                code << "  jz " << ".L" << L << endl;
                break;
            case (Tokentypes::jne):
                code << "  jne " << ".L" << L << endl;
                break;
            case (Tokentypes::jnz):
                code << "  jnz " << ".L" << L << endl;
                break;
            case (Tokentypes::jg):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  ja " << ".L" << L << endl;
                else
                    code << "  jg " << ".L" << L << endl;
                break;
            case (Tokentypes::jnle):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jnbe " << ".L" << L << endl;
                else
                    code << "  jnle " << ".L" << L << endl;
                break;
            case (Tokentypes::jge):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jae " << ".L" << L << endl;
                else
                    code << "  jge " << ".L" << L << endl;
                break;
            case (Tokentypes::jnl):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jnb " << ".L" << L << endl;
                else
                    code << "  jnl " << ".L" << L << endl;
                break;
            case (Tokentypes::jl):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jb " << ".L" << L << endl;
                else
                    code << "  jl " << ".L" << L << endl;
                break;
            case (Tokentypes::jnge):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jnae " << ".L" << L << endl;
                else
                    code << "  jnge " << ".L" << L << endl;
                break;
            case (Tokentypes::jle):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jbe " << ".L" << L << endl;
                else
                    code << "  jle " << ".L" << L << endl;
                break;
            case (Tokentypes::jng):
                if (currentC.left.isUnsigned || currentC.right.isUnsigned)
                    code << "  jna " << ".L" << L << endl;
                else
                    code << "  jng " << ".L" << L << endl;
                break;
            }
        }
        else {
            switch (currentC.op) {
            case (Tokentypes::je):
                code << "  jne " << ".L" << L << endl;
                break;
            case (Tokentypes::jz):
                code << "  jnz " << ".L" << L << endl;
                break;
            case (Tokentypes::jne):
                code << "  je " << ".L" << L << endl;
                break;
            case (Tokentypes::jnz):
                code << "  jz " << ".L" << L << endl;
                break;
            case (Tokentypes::jg):
                code << "  jng " << ".L" << L << endl;
                break;
            case (Tokentypes::jnle):
                code << "  jle " << ".L" << L << endl;
                break;
            case (Tokentypes::jge):
                code << "  jnge " << ".L" << L << endl;
                break;
            case (Tokentypes::jnl):
                code << "  jl " << ".L" << L << endl;
                break;
            case (Tokentypes::jl):
                code << "  jnl " << ".L" << L << endl;
                break;
            case (Tokentypes::jnge):
                code << "  jge " << ".L" << L << endl;
                break;
            case (Tokentypes::jle):
                code << "  jnle " << ".L" << L << endl;
                break;
            case (Tokentypes::jng):
                code << "  jg " << ".L" << L << endl;
                break;
            }
        }

        //swap code and condition code to write conditions
        swap();

        code << ".L" << L << ":" << endl;
    }

    condi:

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
                    if (line[0].type == Tokentypes::condition && line[1].type == Tokentypes::lf) {
                        j++;
                        break;
                    }
                    if (execute) {
                        condi = check_condi(line, _tokens, &j, true);
                        if (condi == false) {
                            interpret_and_compile_var(line);
                            interpret_and_compile(line);
                        }
                    }
                    line.erase(line.begin(), line.end());
                }
                else {
                    line.erase(line.begin(), line.end());
                }
            }
            else line.push_back(_tokens[j]);
        }

        if (execute && !isDynamic) code << "  ret" << endl;

        //swap again to write back to function's code
        if(!isDynamic) swap();

        return j;
    }
    else {
        bool condi = false;
        int i = 0;
        condi = check_condi(line, _tokens, &i, true);
        if (!condi && execute) {
            interpret_and_compile_var(line);
            interpret_and_compile(line);
        }
        line.erase(line.begin(), line.end());
    }

    if (!isDynamic) code << "  ret" << endl;
    //swap again to write back to function's code
    if (!isDynamic) swap();
    return 1;
}

std::array<Tokentypes, 11> conditionOperators = { 
    Tokentypes::je,
    Tokentypes::jne,
    Tokentypes::jg,
    Tokentypes::jng,
    Tokentypes::jge,
    Tokentypes::jnge,
    Tokentypes::jl,
    Tokentypes::jnl,
    Tokentypes::jle,
    Tokentypes::jnle,
    Tokentypes::is
};

void compilation::register_condi(vector<token> line) {
    condition c = {};
    // get condition's name
    c.name = line[1].t; // the name is always after the $ which must be the first token

    int i = 0;
    int operatortoken = -1;

    // get operator
    for (auto t : line) {
        for (auto o : conditionOperators) {
            if (t.type == o) {
                c.op = line[i].type;
                operatortoken = i;
                goto registerCondi_breakall;
            }
        }
        i++;
    }

registerCondi_breakall:

    // get left operand
    // in order to do so we need to know the position of the '=' sign which is right after the name
    vector<token> subline = {token("{browse_argument", line[0].line, Tokentypes::_constant), token("(", line[0].line, Tokentypes::parameter_start)};
    subline.insert(subline.end(), line.begin() + 3, line.begin() + operatortoken);
    subline.push_back(token(")", line[0].line, Tokentypes::parameter_end));
    subline.push_back(token("\n", line[0].line, Tokentypes::lf));
    vector<constant> leftconstant = browse_argument(subline);
    c.left = leftconstant[0];

    // check if we need a second operand

    if (c.op == Tokentypes::jnz) { // =True operator (need only one operand
        c.right = c.left;
    }
    elif(c.op == Tokentypes::jz) { // =False operator (need only one operand
        c.right = c.left;
    }
    else {

        // get right operand

        // check if second operand exist
        if (line[operatortoken].t == "!!!") {
            cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << line[0].line << " condition needs a right operand" << endl;
            std::exit(1);
        }
        // prevent vector error
        if (line[operatortoken].next == nullptr) {
            cerr << Error::errorTypeError << "Error: unexpected eof at line " << line[operatortoken + 1].line << Error::errorTypeNormal << '\n';
            exit(1);
        }

        subline.clear();
        subline = { token("{browse_argument", line[0].line, Tokentypes::_constant), token("(", line[0].line, Tokentypes::parameter_start) };
        subline.insert(subline.end(), line.begin() + operatortoken, line.end());
        subline.push_back(token(")", line[0].line, Tokentypes::parameter_end));
        subline.push_back(token("\n", line[0].line, Tokentypes::lf));
        vector<constant> rightconstant = browse_argument(subline);
        c.right = rightconstant[0];
    }
    // register
    conditions.push_back(c);
}

void compilation::callFunctionFromNameAndParameters(string name, vector<constant> parameters) {
    vector<token> line = { token(name, 0, Tokentypes::_constant), token("(", 0, Tokentypes::parameter_start) };

    int i = 1;
    for (auto c : parameters) {
        if (i == parameters.size()) {
            line.push_back(token(c.name, 0, Tokentypes::_constant));
            line.push_back(token(")", 0, Tokentypes::parameter_end));
            line.push_back(token("\n", 0, Tokentypes::lf));
        }
        else {
            line.push_back(token(c.name));
            line.push_back(token(",", 0, Tokentypes::separator));
        }
        i++;
    }

    interpret_and_compile(line);

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
                        type = getType(_tokens[0]);
                        spaces.push_back(constant(_tokens[i - 1].t, Tokentypes::global_variable, _tokens[i - 1].t, type));
                        section_data += "  " + _tokens[i - 1].t + ": dq " + value;
                        constants.push_back(constant(_tokens[i - 1].t, Tokentypes::global_variable, _tokens[i - 1].t, type));
                    }
                    // in case of crash add else here
                    // (no crashs in demo use found)
                }
                elif(constants[consta].type == Tokentypes::variable) {
                    Basetype type = constants[consta].lastUse;
                    if (i != 1) {
                        type = getType(_tokens[0]);
                        if (type != constants[consta].lastUse) {
                            if (constants[consta].fixedType == false) {
                                if (constants[consta].lastUse != Basetype::_any) warn(Error::typeWarn, _tokens[0].line, stringstream() << "changing type of variable \"" << constants[consta].name << "\" from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!");
                                constants[consta].lastUse = type;
                            }
                            else {
                                cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << " Cannot change type of fix(" << constants[consta].name << ") from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!" << endl;
                                std::exit(1);
                            }
                        }
                    }
                    if (value != constants[consta].reg) {
                        if (type == Basetype::_float) {
                            if (architecture == 64) {
                                code << "  mov rax " << ", [" << value << ']' << endl;
                                code << "  mov qword " << constants[consta].reg << ", rax" << endl;
                            }
                        }
                        else {
                            code << "  mov qword " << constants[consta].reg << ", " << value << endl;
                        }
                    }
                }
                elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                    Basetype type = Basetype::_any;
                    type = getType(_tokens[0]);
                    if (type != constants[consta].lastUse) {
                        if (constants[consta].fixedType == false) {
                            warn(Error::typeWarn, _tokens[0].line, stringstream() << "changing type of variable \"" << constants[consta].name << "\" from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!");
                            constants[consta].lastUse = type;
                        }
                        else {
                            cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << " Cannot change type of fixed " << constants[consta].name << "from " << fromType(constants[consta].lastUse) << " to " << fromType(type) << "!" << endl;
                            std::exit(1);
                        }

                    }
                    if (in(constants[consta].name, REG)) {
                        code << "  mov " << REG[0] << ", " << value << endl;
                        code << "  mov " << constants[consta].name << ", " << REG[0] << endl;
                    }
                    else {
                        code << "  mov " << REG[0] << ", " << value << endl;
                        code << "  mov " << constants[consta].reg << ", " << REG[0] << endl;
                    }

                }
                break;
            }
            else {
                size_t consta = is_registered_constant(_tokens[i - 1].t);
                if (consta == -1) {
                    consta = is_registered_constant(_tokens[i + 1].t);
                    if (consta == -1) {
                        cerr << Error::errorTypeError << "Error at line: " << Error::errorTypeNormal << _tokens[0].line << " cannot use undefined variable '" << _tokens[i - 1].t << "'!" << endl;
                        std::exit(1);
                    }
                    elif(constants[consta].type == Tokentypes::variable) {
                        _tokens.insert(_tokens.begin(), { token(_tokens[i + 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                        string value = browse_value(_tokens);
                        code << "  mov qword  " << constants[consta].reg << ", " << value << endl;
                    }
                    elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                        _tokens.insert(_tokens.begin(), { token(_tokens[i + 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                        string value = browse_value(_tokens);
                        code << "  mov " << REG[0] << ", " << value << endl;
                        code << "  mov " << constants[consta].reg << ", " << REG[0] << endl;
                    }
                }
                elif(constants[consta].type == Tokentypes::variable) {
                    _tokens.insert(_tokens.begin(), { token(_tokens[i - 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                    string value = browse_value(_tokens);
                    code << "  mov qword  " << constants[consta].reg << ", " << value << endl;
                }
                elif(constants[consta].type == Tokentypes::global_variable || constants[consta].type == Tokentypes::_extern) {
                    _tokens.insert(_tokens.begin(), { token(_tokens[i - 1].t, _tokens[0].line, Tokentypes::equal), token("=", _tokens[0].line, Tokentypes::equal) });
                    string value = browse_value(_tokens);
                    code << "  mov " << REG[0] << ", " << value << endl;
                    code << "  mov " << constants[consta].reg << ", " << REG[0] << endl;
                }
                break;
            }
        }
    }
    return code.str();
}

void compilation::interpret_and_compile(vector<token>& _tokens) {
    string fname = "";
    bool isChild = false;
    bool isMold = false;
    constant childCaller;

    bool hasParenthesis = false;
    for (auto t : _tokens) {
        if (t.type == Tokentypes::parameter_start) {
            hasParenthesis = true;
            break;
        }
    }

    if (!hasParenthesis) return;

    for (size_t i = 0; i < _tokens.size(); i++) {
        if (_tokens[i].type == Tokentypes::_constant || _tokens[i].t == "int") {
            if (_tokens[i + 1].type == Tokentypes::parameter_start) {
                size_t consta = is_registered_constant(_tokens[i].t);
                for (auto m : preproc.molds) {
                    if (m.name == _tokens[i].t) {
                        isMold = true;
                        break;
                    }
                }
                if (consta != -1 || isMold) {
                    if (isMold) {
                        fname = _tokens[i].t;
                        break;
                    }
                    if (constants[consta].type == Tokentypes::function || constants[consta].type == Tokentypes::_extern) {
                        if (_tokens[i + 1].type == Tokentypes::parameter_start) fname = constants[consta].name;
                        break;
                    }
                    else {
                        if (detect_constant_type(_tokens[i], _tokens) == Tokentypes::function) {
                            cerr << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": function " << _tokens[i].t << " called but not created";
                            std::exit(1);
                        }
                    }
                }
                else {
                    cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << ": Cannot call non existant function \"" << _tokens[i].t << "\" !" << endl;
                    std::exit(1);
                }
            }
            elif(_tokens[i].next->type == Tokentypes::float_separator) {
                isChild = true;
                bool registered = 0;

                // is not a float?
                if (is_float(token(_tokens[i].t + _tokens[i + 1].t + _tokens[i + 2].t))) break;

                // is registered child?
                for (auto j : childs) {
                    registered |= (j.name == _tokens[i].next->next->t);
                }
                for (auto j : preproc.childs) {
                    registered |= (j.name == _tokens[i].next->next->t);
                }

                if (registered) {
                    fname = _tokens[i].next->next->t;
                    constant* caller = get_registered_constant(_tokens[i].t);
                    if (caller == nullptr) {
                        cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << ": Cannot call from non existant constant \"" << _tokens[i].t << "\" !" << endl;
                        exit(1);
                    }
                    childCaller = *caller;
                    break;
                }
                elif(detect_constant_type(_tokens[i], _tokens) == Tokentypes::function) {
                    cerr << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": child " << _tokens[i].t << " called but not created";
                    std::exit(1);
                }
                else {
                    cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << ": Cannot call non existant function \"" << _tokens[i].t << "\" !" << endl;
                    std::exit(1);
                }
            }
        }
    }
    if (fname != "") {
        if (fname == "return") {
            vector<constant> parameters = browse_argument(_tokens);
            if (parameters.size() > 0) {
                if (parameters[0].reg != "") {
                    code << "  mov " << REG[0] << ", " << parameters[0].reg << endl;
                    code << "  add " << REG[7] << ", " << this->stack + (16 * 2 + 8) << endl;
                    code << "  mov " << REG[7] << ", " << REG[6] << endl;
                    code << "  pop " << REG[6] << endl;
                    code << "  ret" << endl;
                }
                else {
                    code << "  mov " << REG[0] << ", " << parameters[0].name << endl;
                    code << "  add " << REG[7] << ", " << this->stack + (16 * 2 + 8) << endl;
                    code << "  mov " << REG[7] << ", " << REG[6] << endl;
                    code << "  pop " << REG[6] << endl;
                    code << "  ret" << endl;
                }
            }
            else {
                code << "  add " << REG[7] << ", " << this->stack + (16 * 2 + 8) << endl;
                code << "  ret" << endl;
            }
        }
        elif(isChild) {

            bool bIsAsChild = 0;
            bool bIsLsChild = 0;
            // is asm or L# child?
            for (auto j : childs) {
                bIsAsChild |= (j.name == fname);
            }
            for (auto j : preproc.childs) {
                bIsLsChild |= (j.name == fname);
            }

            vector<constant> parameters = { childCaller };
            vector<constant> arguments = browse_argument(_tokens);
            parameters.insert(parameters.end(), arguments.begin(), arguments.end());

            if (bIsAsChild) {
                aschild reference;

                // search for the called child (we know it is created!)
                for (auto c : childs) {
                    if (c.name == fname) {
                        reference = c;
                    }
                }

                // verify parameters

                for (int i = 0; i < reference.parameters.size(); i++) {
                    if (parameters[i].lastUse != reference.parameters[i].lastUse &&
                        parameters[i].lastUse != Basetype::_any &&
                        reference.parameters[i].lastUse != Basetype::_any) {
                        cerr << Error::errorTypeError << "Error at line:" << _tokens[0].line << Error::errorTypeNormal << " could not find any child for type " << fromType(parameters[i].lastUse) << " at paremeter " << i << "!" << endl;
                        cerr << "Candidate is: " << fromType(reference.parameters[i].lastUse) << '\n';
                        std::exit(1);
                    }
                }

                for (int i = 0; i < reference.parameters.size(); i++) {
                    if (parameters[i].reg != "") {
                        if (!replace(reference.code, reference.parameters[i].name, parameters[i].reg)) {
                            cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in child " << reference.name << " parameter " << i << " cannot be found";
                            std::exit(1);
                        }
                    }
                    else {
                        if (!replace(reference.code, reference.parameters[i].name, parameters[i].name)) {
                            cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in child " << reference.name << " parameter " << i << " cannot be found";
                            std::exit(1);
                        }
                    }
                }

                code << reference.fcode(2);
            }
            elif (bIsLsChild) {
                lschild reference;

                // search for the called child (we know it is created!)
                int offset = 0;
                for (auto c : preproc.childs) {
                    if (c.name == fname) {
                        reference = c;
                        break;
                    }
                    offset++;
                }

                // verify parameters

                for (int i = 0; i < reference.parameters.size(); i++) {
                    if (parameters[i].lastUse != reference.parameters[i].lastUse &&
                        parameters[i].lastUse != Basetype::_any &&
                        reference.parameters[i].lastUse != Basetype::_any) {
                        cerr << Error::errorTypeError << "Error at line:" << _tokens[0].line << Error::errorTypeNormal << " could not find any child for type " << fromType(parameters[i].lastUse) << " at paremeter " << i << "!" << endl;
                        cerr << "Candidate is: " << fromType(reference.parameters[i].lastUse) << '\n';
                        std::exit(1);
                    }
                }

                code << compile_mold(false, true, false, offset, __tokens, parameters);
            }

        }
        elif(isMold) {

            vector<constant> parameters = { };
            vector<constant> arguments = browse_argument(_tokens);
            parameters.insert(parameters.end(), arguments.begin(), arguments.end());

            mold reference;

            //parameters = browse_argument(_tokens, true);

            // search for the called mold (we know it is created!)
            int offset = 0;
            for (auto c : preproc.molds) {
                if (c.name == fname) {
                     reference = c;
                    break;
                }
                offset++;
            }

            // verify parameters
            for (int i = 0; i < reference.parameters.size(); i++) {
                if (parameters[i].lastUse != reference.parameters[i].lastUse &&
                    parameters[i].lastUse != Basetype::_any &&
                    reference.parameters[i].lastUse != Basetype::_any) {
                    cerr << Error::errorTypeError << "Error at line:" << _tokens[0].line << Error::errorTypeNormal << " could not find any mold for type " << fromType(parameters[i].lastUse) << " at paremeter " << i << "!" << endl;
                    cerr << "Candidate is: " << fromType(reference.parameters[i].lastUse) << '\n';
                    std::exit(1);
                }
            }

            code << compile_mold(true, false, false, offset, __tokens, parameters);
        }
        elif(fname == "nop") {
            code << "  nop" << endl;
        }
        elif(fname == "int") {
            vector<constant> parameters = browse_argument(_tokens, true);
            if (parameters[0].reg != "") code << "  int " << parameters[0].reg << endl;
            else code << "  int " << parameters[0].name << endl;
        }
        elif(fname == "fix") {
            vector<constant> parameters = browse_argument(_tokens);
            for (int i = 0; i < parameters.size(); i++) {
                constant* c = &constants[is_registered_constant(parameters[i].name)];
                if (c != nullptr) {
                    c->fixedType = true;
                }
                else {
                    cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " Cannot fix the type of \"" << parameters[i].name << "\"!" << endl;
                }
            }
        }
        elif(fname == "unsign") {
            vector<constant> parameters = browse_argument(_tokens);
            for (int i = 0; i < parameters.size(); i++) {
                constant* c = &constants[is_registered_constant(parameters[i].name)];
                if (c != nullptr) {
                    if (c->fixedType == false) {
                        c->isUnsigned = true;
                    }
                    else {
                        cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << " Cannot change type of fix(" << c->name << ")" << " to unsigned !" << endl;
                        std::exit(1);
                    }
                }
                else {
                    cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " Cannot fix the type of \"" << parameters[i].name << "\"!" << endl;
                }
            }
        }
        elif(fname == "sign") {
            vector<constant> parameters = browse_argument(_tokens);
            for (int i = 0; i < parameters.size(); i++) {
                constant* c = &constants[is_registered_constant(parameters[i].name)];
                if (c != nullptr) {
                    if (c->fixedType == false) {
                        c->isUnsigned = false;
                    }
                    else {
                        cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[0].line << " Cannot change type of fix(" << c->name << ")" << " to signed !" << endl;
                        std::exit(1);
                    }
                }
                else {
                    cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " Cannot fix the type of \"" << parameters[i].name << "\"!" << endl;
                }
            }
        }
        elif(fname == "is") {
            vector<constant> parameters = browse_argument(_tokens, true);

            if (parameters.size() != 2) {
                cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " is() takes exactly two arguments !" << endl;
                std::exit(1);
            }

            constant* c = &constants[is_registered_constant(parameters[1].name)];
            if (c == nullptr) {
                cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " Cannot get attribute of \"" << parameters[1].name << "\"!" << endl;
                std::exit(1);
            }
            if (parameters[0].name == "UNSIGNED") {
                code << "  mov " << REG[0] << ", " << (c->isUnsigned) << endl;
            }
            elif(parameters[0].name == "SIGNED") {
                code << "  mov " << REG[0] << ", " << (!c->isUnsigned) << endl;
            }
            elif(parameters[0].name == "int") {
                code << "  mov " << REG[0] << ", " << (c->lastUse == Basetype::_int) << endl;
            }
            elif(parameters[0].name == "float") {
                code << "  mov " << REG[0] << ", " << (c->lastUse == Basetype::_float) << endl;
            }
            elif(parameters[0].name == "str") {
                code << "  mov " << REG[0] << ", " << (c->lastUse == Basetype::_string) << endl;
            }
            elif(parameters[0].name == "ptr") {
                code << "  mov " << REG[0] << ", " << (c->lastUse == Basetype::_ptr) << endl;
            }
            else {
                cerr << Error::errorTypeError << "Error at line: " << _tokens[0].line << Error::errorTypeNormal << " Invalid attrubute \"" << parameters[0].name << "\" !" << endl;
                std::exit(1);
            }
        }
        elif(fname == "syscall") {
            code << "  syscall" << endl;
        }
        elif(fname == "label") {
            vector<constant> parameters = browse_argument(_tokens, true);
            code << "." << parameters[0].name << ":" << endl;
        }
        elif(fname == "goto") {
            vector<constant> parameters = browse_argument(_tokens, true);
            code << "  jmp ." << parameters[0].name << endl;
        }
        else {
            vector<constant> parameters = browse_argument(_tokens);

            int to_add = 0;

            if (convention != CallingConvention::___cdecl) {
                reverse(parameters.begin(), parameters.end());
            }
            else {
                if (parameters.size() % 2 == 1) to_add = 8;
                code << "  sub " << REG[7] << ", " << 8 * parameters.size() << '\n';
            }

            size_t p_num = 0;
            size_t fp_num = 0;
            for (constant i : parameters) {
                if (p_num + fp_num < argument_order.size()) {
                    if (i.lastUse == Basetype::_float) {
                        code << "  movq " << REG[fargument_order[fp_num]] << ", " << i.reg << "\n";
                        fp_num++;
                    }
                    else {
                        if (i.reg.rfind("LC", 0) == 0) code << "  lea " << REG[argument_order[p_num]] << ", [rel " << i.reg << "]" << endl;
                        elif(i.reg != "") code << "  mov " << REG[argument_order[p_num]] << ", " << i.reg << endl;
                        else code << "  mov " << REG[argument_order[p_num]] << ", " << i.name << endl;
                        p_num++;
                    }
                }
                else {
                    if (convention != CallingConvention::___cdecl) {
                        if (i.reg != "") code << "  push qword " << i.reg << endl;
                        else code << "  push " << i.name << endl;
                    }
                    else {
                        i = parameters[parameters.size() - 1 - p_num + 4];
                        if (i.reg != "") code << "  mov qword [" << REG[7] << " + " << (parameters.size() - 1 - p_num + 4) * 8 << "], " << i.reg << endl;
                        else code << "  mov qword [" << REG[7] << " + " << (parameters.size() - 1 - p_num + 4) * 8 << "] , " << i.name << endl;
                    }
                }
            }
            code << "  xor " << REG[0] << ", " << REG[0] << endl;
            bool isExtern = get_registered_constant(fname)->type == Tokentypes::_extern;
            if (isExtern) {
                code << "  call default rel " << fname << endl;
            }
            else {
                code << "  call " << fname << endl;
            }
            if (convention == CallingConvention::___cdecl) {
                code << "  add " << REG[7] << ", " << 8 * parameters.size() << '\n';
            }
        }
    }

    return;
}

void compilation::add_external_constant(constant c) {
    if (c.type == Tokentypes::function || c.type == Tokentypes::global_variable) {
        if (is_registered_constant(c.name) == -1)
            extConstants.push_back(c);
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
            elif(is_float(_tokens[i])) {
                ROspaces += 1;
                section_data += "  LC" + to_string(ROspaces) + ": dq " + _tokens[i].t + '\n';
                return "LC" + to_string(ROspaces);
            }
            elif(is_string(_tokens[i])) {
                string value;
                if (is_global == false) {
                    ROspaces += 1;
                    string processedStr = formatString(_tokens[i].t);
                    section_data += "  LC" + to_string(ROspaces) + ": db " + processedStr + '\n';
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
                    cerr << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": constant " << _tokens[i].t << " used but not defined";
                    std::exit(1);
                }
                string value;
                if (constants[consta].type == Tokentypes::function) {
                    value = REG[0];
                }
                else {
                    if (constants[consta].reg != "") {
                        code << "  mov " << REG[0] << ", " << constants[consta].reg << endl;
                        value = REG[0];
                    }
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

void compilation::getStack(vector<token>& tokens) {
    // get all variables of the function, define them at the top of the function, reserve the stack and protect it
    vector<token> line;

    bool isfirstline = true;
    bool icondi = false;
    bool parenthesis = false;
    bool outsidefunction = true;

    stringstream parametersCode;
    parametersCode.str("");
    int smallvarSpace = 0;

    // iterate through function's tokens
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::colon) outsidefunction = false;
        if (tokens[i].type == Tokentypes::parameter_start) parenthesis = true;
        if (tokens[i].type == Tokentypes::parameter_end) parenthesis = false;
        if (tokens[i].type == Tokentypes::_constant && parenthesis && outsidefunction) {
            if (tokens[i - 1].type == Tokentypes::type) {
                if (getType(tokens[i - 1]) != Basetype::customStructure) {
                    parametersCode << "  mov qword [" << REG[6] << " - " << to_string(this->stack) << "], " << REG[argument_order[this->stack / 8]] << "\n";
                    constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", getType(tokens[i - 1])));
                    this->stack += 8;
                }
                else {
                    structure c_struct;
                    for (auto s : structs) {
                        if (s.name == tokens[i - 1].t) {
                            c_struct = s;
                            break;
                        }
                    }

                    constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", getType(tokens[i - 1])));

                    for (auto v : c_struct.vars) {
                        parametersCode << "  mov qword [" << REG[6] << " - " << to_string(this->stack) << "], " << REG[argument_order[this->stack / 8]] << "\n";
                        constants.push_back(constant(tokens[i].t + "." + v.first, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", getType(tokens[i - 1])));
                        this->stack += 8;
                    }
                }
            }
            else {
                parametersCode << "  mov qword [" << REG[6] << " - " << to_string(this->stack) << "], " << REG[argument_order[this->stack / 8]] << "\n";
                constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", Basetype::_any));
                this->stack += 8;
            }
        }
        if (tokens[i].type == Tokentypes::lf) {
            if (!isfirstline) {
                line.push_back(tokens[i]);
                if (line[0].type == Tokentypes::condition || line[0].type == Tokentypes::inv) {
                    icondi = true;
                }
                else {
                    for (size_t i = 0; i < line.size(); i++) {
                        if (line[i].type == Tokentypes::equal) {
                            size_t consta = is_registered_constant(line[i - 1].t);
                            string value = "0";
                            if (consta == -1) {
                                if (detect_constant_type(line[i - 1], line) == Tokentypes::global_variable) {
                                    value = browse_value(line, true);
                                    Basetype type = Basetype::_any;
                                    type = getType(line[0]);
                                    spaces.push_back(constant(line[i - 1].t, Tokentypes::global_variable, line[i - 1].t, type));
                                    section_data += "  " + line[i - 1].t + ": dq " + value;
                                    constants.push_back(constant(line[i - 1].t, Tokentypes::global_variable, line[i - 1].t, type));
                                }
                                else {
                                    string reg = get_reg();
                                    Basetype type = Basetype::_any;
                                    int lastSmallvarSpace = smallvarSpace;
                                    if (type_dict.find(line[0].t) != type_dict.end()) {
                                        type = getType(line[0]);
                                        if (type == Basetype::_bool)        smallvarSpace += 1;
                                        else if (type == Basetype::_2bool)  smallvarSpace += 2;
                                        else if (type == Basetype::_3bool)  smallvarSpace += 3;
                                        else if (type == Basetype::_4bool)  smallvarSpace += 4;
                                        else if (type == Basetype::_5bool)  smallvarSpace += 5;
                                        else if (type == Basetype::_6bool)  smallvarSpace += 6;
                                        else if (type == Basetype::_7bool)  smallvarSpace += 7;
                                        else if (type == Basetype::_byte)   smallvarSpace += 8;
                                        else if (type == Basetype::_word)   smallvarSpace += 16;
                                        else if (type == Basetype::_dword)  smallvarSpace += 32;
                                        else if (type == Basetype::_qword)  smallvarSpace += 64;
                                        else if (type == Basetype::customStructure) {
                                            structure c_struct;

                                            for (auto s : structs) {
                                                if (s.name == line[0].t) {
                                                    c_struct = s;
                                                    break;
                                                }
                                            }

                                            for (auto v : c_struct.vars)
                                                constants.push_back(constant(line[1].t + "." + v.first, Tokentypes::variable, get_reg(), v.second));
                                        }
                                    }
                                    if (lastSmallvarSpace - smallvarSpace != 0) { // is a smallvar
                                        constants.push_back(constant(line[i - 1].t, Tokentypes::variable, reg, type, true, lastSmallvarSpace - smallvarSpace));
                                    }
                                    else { // is not a smallvar
                                        constants.push_back(constant(line[i - 1].t, Tokentypes::variable, reg, type));
                                    }
                                }
                            }
                            break;
                        }
                    }
                    line.erase(line.begin(), line.end());
                }
            }
            else {
                isfirstline = false;
            }
        }
        else line.push_back(tokens[i]);
    }
    
    if (architecture == 32) code << "  and " << REG[7] << ", -16\n";
    if (this->stack % 16 == 8) this->stack += 8;
    code << "  sub " << REG[7] << ", " << this->stack + (16 * 2 + 8) << '\n';

    code << parametersCode.str();
    parametersCode.clear();
    return;
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
                ROspaces += 1;
                section_data += "  LC" + to_string(ROspaces) + ": dq " + _tokens[i].t + '\n';
                one = constant("[LC" + to_string(ROspaces) + "]", Tokentypes::_float, "[LC" + to_string(ROspaces) + "]", Basetype::_float);
                bone = false;
            }
            else {
                ROspaces += 1;
                section_data += "  LC" + to_string(ROspaces) + ": dq " + _tokens[i].t + '\n';
                two = constant("[LC" + to_string(ROspaces) + "]", Tokentypes::_float, "[LC" + to_string(ROspaces) + "]", Basetype::_float);
                bone = true;
            }
        }
        elif(is_string(_tokens[i])) {
            ROspaces += 1;
            string processedStr = formatString(_tokens[i].t);
            section_data += "  LC" + to_string(ROspaces) + ": db" + processedStr + "\n";
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
                cerr << Error::errorTypeError << "Error" << " at line " << Error::errorTypeNormal << _tokens[i].line << ": constant " << _tokens[i].t << " used but not defined";
                std::exit(1);
            }
            string value;
            if (constants[consta].reg != "") {
                if (bone) {
                    one = constants[consta];
                    one.name = "" + one.reg;
                    one.reg = "" + one.reg;
                    bone = false;
                }
                else {
                    two = constants[consta];
                    two.name = "" + two.reg;
                    two.reg = "" + two.reg;
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
            asoperator ope;
            for (asoperator o : operators) {
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
                cerr << Error::errorTypeError << "Error at line:" << _tokens[0].line << Error::errorTypeNormal << " could not find any operators for types " << fromType(one.lastUse) << " and " << fromType(two.lastUse) << "!" << endl;
                std::exit(1);
            }

            if (ope.parameters.size() < 2 || ope.parameters.size() > 2) {
                cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameters lenght not equal to two";
                std::exit(1);
            }

            if (!replace(ope.code, ope.parameters[0].name, two.name)) {
                cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameter 0 cannot be found";
                std::exit(1);
            }

            if (!replace(ope.code, ope.parameters[1].name, one.name)) {
                cerr << Error::errorTypeError << "Error" << Error::errorTypeNormal << " in operator " << ope.name << " parameter 1 cannot be found";
                std::exit(1);
            }

            code << ope.fcode(2);
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

string compilation::compile_mold(bool isMold, bool isChild, bool isOperator, int offset, vector<token>& _tokens, vector<constant> passedParameters) {
    string name = "";
    vector<token> tokens = {};
    vector<constant> p = {};
    vector<constant> funcConstants = {};
    vector<condition> funcConditions = {};
    stringstream code2;
    bool is_first_line = true;

    if (isMold) {
        tokens.insert(tokens.begin(), preproc.molds[offset].code.begin(), preproc.molds[offset].code.end());
    } elif (isChild) {
        tokens.insert(tokens.begin(), preproc.childs[offset].code.begin(), preproc.childs[offset].code.end());
    } elif(isOperator) {
        tokens.insert(tokens.begin(), preproc.operators[offset].code.begin(), preproc.operators[offset].code.end());
    }
    else {
        cerr << Error::errorTypeError << "internal error : " << __FUNCTION__ << " " << __FILE__ << " isMold, isChild and isOperator are all false!\n" << Error::errorTypeNormal;
    }

    funcConstants.insert(funcConstants.end(), constants.begin(), constants.end());
    funcConditions.insert(funcConditions.end(), conditions.begin(), conditions.end());
    constants.clear();
    conditions.clear();

    constants.insert(constants.end(), extConstants.begin(), extConstants.end());
    constants.insert(constants.end(), spaces.begin(), spaces.end());
    register_function(_tokens);

    bool parenthesis = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::parameter_start) parenthesis = true;
        if (tokens[i].type == Tokentypes::parameter_end) {
            parenthesis = false;
            break;
        }
        if (tokens[i].type == Tokentypes::_constant && tokens[i + 1].type == Tokentypes::parameter_start) {
            name = tokens[i].t;
        }
        if (tokens[i].type == Tokentypes::_constant && parenthesis) {
            if (tokens[i - 1].type == Tokentypes::type && getType(tokens[i - 1]) != Basetype::_any) {
                p.push_back(constant(tokens[i].t, Tokentypes::variable, passedParameters[p.size()].reg, getType(tokens[i - 1])));
            }
            else {
                p.push_back(constant(tokens[i].t, Tokentypes::variable, passedParameters[p.size()].reg, passedParameters[p.size()].lastUse));
            }
        }
    }

    size_t constsize = constants.size();

    constants.insert(constants.end(), p.begin(), p.end());

    if (name == "") {
        cerr << Error::errorTypeError << "Error: Function has no name at line " << Error::errorTypeNormal << tokens[0].line << " maybe you forgot `('?\n";
    }

    vector<token> line;

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

    constants.clear();
    constants.insert(constants.end(), funcConstants.begin(), funcConstants.end());

    conditions.clear();
    conditions.insert(conditions.begin(), funcConditions.begin(), funcConditions.end());

    return code2.str();
}

string compilation::compile_function(int function_start, vector<token>& _tokens) {
    string name = "";
    vector<token> tokens;
    bool is_first_line = true;

    // getting the function's tokens
    for (int i = function_start; i < _tokens.size(); i++) {
        if (_tokens[i].type != Tokentypes::end) {
            tokens.push_back(_tokens[i]);
        }
        else {
            break;
        }
    }

    // clear local variables of previous functions
    constants.clear();
    conditions.clear();
    this->stack = 8;
    // add the external constants
    constants.insert(constants.end(), extConstants.begin(), extConstants.end());
    constants.insert(constants.end(), spaces.begin(), spaces.end());
    register_function(_tokens);

    for (string reg : REG) {
        constants.push_back(constant(reg, Tokentypes::global_variable, "", Basetype::_any));
    }

    bool parenthesis = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::parameter_start) parenthesis = true;
        if (tokens[i].type == Tokentypes::parameter_end) {
            parenthesis = false;
            break;
        }
        if (tokens[i].type == Tokentypes::definition) {
            switch (tokens[i + 1].type) {
                case Tokentypes::mold_definition:
                    return "";
                case Tokentypes::child:
                    return "";
                case Tokentypes::_operator:
                    return "";
            }
        }
        if (tokens[i].type == Tokentypes::_constant && tokens[i + 1].type == Tokentypes::parameter_start) {
            name = tokens[i].t;
            currentFname = name;
            code.str("");

            code << "GLOBAL " << name << endl;
            code << name << ":" << endl;
            code << "  push " << REG[6] << endl;
            code << "  mov " << REG[6] << ", " << REG[7] << endl;
            
            getStack(tokens);
        }
    }

    if (currentFname == "") {
        cerr << Error::errorTypeError << "Error: Function has no name at line " << Error::errorTypeNormal << tokens[0].line << " maybe you forgot `('?\n";
        std::exit(1);
    }

    vector<token> line;

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

    return code.str() + conditionCode.str();
}

vector<int> get_functions(vector<token> tokens) {
    vector<int> functions;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::definition) {
            if (tokens[i + 1].type == Tokentypes::_constant) {
                functions.push_back(i);
            }
            elif(tokens[i + 1].type == Tokentypes::child ||
                tokens[i + 1].type == Tokentypes::mold_definition ||
                tokens[i + 1].type == Tokentypes::_operator) {
                if (tokens[i + 2].type == Tokentypes::_constant) {
                    functions.push_back(i);
                }
            }
        }
    }
    return functions;
}

vector<int> get_structures(vector<token> tokens) {
    vector<int> structs;
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == Tokentypes::_struct) {
            if (tokens[i + 1].type == Tokentypes::_constant) {
                structs.push_back(i);
            }
        }
    }
    return structs;
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
    if (_tokens[index + 1].type == Tokentypes::child) return true;
    return false;
}

string precompilation::precompile_lib(vector<token>& _tokens) {
    size_t index = 0;
    bool start = false;
    bool bop = false;
    bool bchild = false;
    bool bfunc = false;
    vector<constant> arguments;
    asoperator op;
    aschild ch;
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

                op.isPrecompiled = true;

                bop = true;
            }
            elif(is_child(_tokens, index)) {
                ch.type = Basetype::_any;

                if (t.type == Tokentypes::type) {
                    ch.type = getType(t);
                }

                // getting operator's type

                // unify operator name (x tokens not only one)
                // searching for the open parenthesis token
                int parenthesis = index + 1;
                while (_tokens[parenthesis++ + 1].type != Tokentypes::parameter_start)
                    ch.name += _tokens[parenthesis].t;

                vector<token> toks;
                toks.insert(toks.end(), _tokens.begin() + index, _tokens.end());

                arguments = browse_parameters(toks);

                for (constant p : arguments) {
                    ch.parameters.push_back(p);
                }

                ch.isPrecompiled = true;

                bchild = true;
                //exit(1);
            }
            elif (!bop && !bchild){
                cerr << Error::errorTypeError << "Error at line " << Error::errorTypeNormal << _tokens[index].line << " : precompiled functions not implemented" << endl;
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
                ch.code = code.str();
                childslib.push_back(ch);
                ch.name = "";
                ch.parameters.clear();
                ch.code = "";
            }
            else {
                cout << Error::errorTypeWarn << "Error:" << Error::errorTypeNormal << " precompiled_functions not implemented!\n";
                std::exit(1);
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