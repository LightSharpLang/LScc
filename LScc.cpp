#include <fstream>
#include <cstdlib>
#include "compilation.h"

/*
todo:

fixed types

signed unsigned (£ = unsigned)

type child token(type...)

struct token

typedef token1 token2

preprocessors

*/

std::map < string, Basetype > type_dict = {
    { "float", Basetype::_float },
    { "int", Basetype::_int },
    { "str", Basetype::_string },
    { "bool", Basetype::_bool },
    { "ptr", Basetype::_ptr },
    { "any", Basetype::_any }
};

std::map < string, Tokentypes > tokens_dict = {
    { "def", Tokentypes::definition },
    { "end", Tokentypes::end },
    { "$", Tokentypes::condition },
    { "[", Tokentypes::list_start },
    { "]", Tokentypes::list_end },
    { "(", Tokentypes::parameter_start },
    { ")", Tokentypes::parameter_end },
    { ",", Tokentypes::separator },
    { ".", Tokentypes::float_separator },
    { ";", Tokentypes::lf },
    { "\\", Tokentypes::line_concat },
    { "global", Tokentypes::global },
    { "local", Tokentypes::local },
    { "\n", Tokentypes::lf },
    { ":", Tokentypes::colon },
    { "extern", Tokentypes::_extern },
    { "with", Tokentypes::with },
    { "#", Tokentypes::comment },
    { "operator", Tokentypes::_operator },
    { "child", Tokentypes::child },
    { "struct", Tokentypes::_struct},
    { "=", Tokentypes::equal },
    { "==", Tokentypes::je },
    { "!=" , Tokentypes::jne },
    { "=_", Tokentypes::jz },
    { "!_", Tokentypes::jnz },
    { ">", Tokentypes::jg },
    { "!>", Tokentypes::jng},
    { ">=", Tokentypes::jge },
    { "!>=", Tokentypes::jnge },
    { "<", Tokentypes::jl },
    { "!<", Tokentypes::jnl },
    { "<=", Tokentypes::jle },
    { "!<=", Tokentypes::jnle },
    { "!", Tokentypes::inv }
    };

int ROspaces = 0;
string section_data = "SECTION .data:\n";
string section_text = "SECTION .text:\n";
vector<int> argument_order;
vector<string> REG;
vector<constant> spaces;
vector<coperator> operators;
vector<string> externs;
vector<string> pcllibs;

string fcout(string str) {
    string ostr = "";
    for (char chr : str) {
        if (chr < 20) {
            ostr += "\\" + to_string((int)chr);
        }
        else ostr += chr;
    }
    return ostr;
}

void print_t_array(const vector<token> a, std::ostream& o)
{
    size_t N = a.size();
    if (N > 0) {
        o << "{";
        for (std::size_t i = 0; i < N - 1; ++i)
        {
            o << fcout(a[i].t) << ", ";
        }
        o << fcout(a[N - 1].t) << "}\n";
    }
    else {
        o << "{}\n";
    }
}

int main(int argc, char** argv)
{
    string file;
    string ofile;
    string afile;
    string Fformat;
    string Cconvention;
    bool save = false;
    if (argc == 1) {
        cout << Error::errorTypeError << "LScc: fatal: no input file specified" << endl << Error::errorTypeNormal
    << "\
    Usage: LScc[options...]                                             \n\
                                                                        \n\
    Options :                                                           \n\
    ----compilation options----                                         \n\
    -i    input file                                                    \n\
    -o    output file | default: input file.obj                         \n\
    -cc   calling convention[SysVi386, SysV, M64] | default: SysV       \n\
    -f    format[elf32, elf64, elfx32, win32, win64] | default: elf64   \n\
    -n    no start function                                             \n\
    -s    save asm file                                                 \n\
                                                                        \n\
    ----warnings options----                                            \n\
    --w-type    disable type convertion warnings                        \n";
    }
    else {
        size_t in1 = in("-i", argv, argc);
        if (in1 != -1) {
            file = argv[in1 + 1];
        }
        else {
            cout << "Error: no input file specified!" << endl;
            // exit(1);
        }

        in1 = in("-f", argv, argc);
        if (in1 != -1) {
            Fformat = argv[in1 + 1];
        }
        else {
            Fformat = "elf64";
        }

        in1 = in("-o", argv, argc);
        if (in1 != -1) {
            ofile = argv[in1 + 1];
            afile = string(file.substr(0, file.rfind('.'))) + ".asm";
        }
        else {
            string filewoext = file.substr(0, file.rfind('.'));
            ofile = filewoext + ".obj";
            afile = filewoext + ".asm";
        }

        in1 = in("-s", argv, argc);
        if (in1 != -1) {
            save = true;
        }

        in1 = in("-cc", argv, argc);
        if (in1 != -1) {
            Cconvention = argv[in1 + 1];
        }
        else {
            Cconvention = "SysV";
        }

        in1 = in("--w-type", argv, argc);
        if (in1 != -1) {
            Error::enabledWarns[Error::typeWarn] = false;
        }

        if (Fformat == "elf64" || Fformat == "win64") {
            REG.insert(REG.end(), { "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3", "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8" });
        }
        elif(Fformat == "elf32" || Fformat == "win32" || Fformat == "elfx32") {
            REG.insert(REG.end(), { "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8"});
        }
        else {
            cout << "Error: invalid format \"" << Fformat << "\" !" << endl;
            exit(1);
        }

        if (Cconvention == "M64") {
            argument_order.insert(argument_order.end(), { 2, 3, 8, 9 });
        }
        elif(Cconvention == "SysV") {
            argument_order.insert(argument_order.end(), { 5, 4, 3, 2, 8, 9 });
        }
        elif(Cconvention == "SysVi386") {
            argument_order.clear();
        }
        else {
            cout << "Error: invalid calling convention \"" << Cconvention << "\" !" << endl;
            exit(1);
        }

        vector<token> tokens = tokenize(file);

        check_pcllibs(tokens);

        precompilation clib;
        vector<token> tokenlibs;
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
        }

        fuse_symbols(&tokens);
        identify_tokens(&tokens);
        check_externs(tokens);
        
        for (string& e : externs) {
            section_text += "extern " + e + "\n";
        }

        compilation c;
        vector<int> functions = get_functions(tokens);
        sort(functions.begin(), functions.end());
        functions.erase(unique(functions.begin(), functions.end()), functions.end());

        for (int f : functions) {
            section_text += c.compile_function(f, tokens);
        }

        string code = section_text + section_data;

        cout << code << endl;

        ofstream asFile(string(afile).c_str());

        asFile << code;

        asFile.close();

        system(string("nasm " + afile + " -f " + Fformat + " -o " + ofile).c_str());

        if (!save) {
            remove(afile.c_str());
        }

        cout << Error::errorTypeValid << "Successfully compiled \x1B[33m\"" << file << "\"\x1B[32m !" << Error::errorTypeNormal << endl;

    }

    return 0;
}