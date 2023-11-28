#include "lscc.h"

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#else
#error "compiler must be at c++17 to compile!"
#endif
/*
todo:

fixed types

signed unsigned (£ = unsigned)

type child token(type...)

struct token

typedef token1 token2

preprocessors

*/

map < string, Basetype > type_dict = {
    { "float", Basetype::_float },
    { "int", Basetype::_int },
    { "str", Basetype::_string },
    { "bool", Basetype::_bool },
    { "ptr", Basetype::_ptr },
    { "any", Basetype::_any }
};

map < string, Tokentypes > tokens_dict = {
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
    { "include", Tokentypes::_include },
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
vector<constant> externs;
vector<string> includes_f;
vector<string> pcllibs;

map<LsccArg, string> Args{
    {LsccArg::i, "" },
    {LsccArg::o, "" },
    {LsccArg::f, "" },
    {LsccArg::cc, "" },
};

filesystem::path WorkingDirectory;

int main(int argc, char** argv)
{
    string afile;
    if (argc == 1) {
        cout << Error::errorTypeError << "LScc: fatal: no input file specified" << endl << Error::errorTypeNormal
    << "\
    Usage: LScc [file] [options...]                                             \n\
                                                                        \n\
    Options :                                                           \n\
    ----compilation options----                                         \n\
    -o    output file | default: input file.obj                         \n\
    -cc   calling convention[SysVi386, SysV, M64]                       \n\
    -f    format[elf32, elf64, elfx32, win32, win64]                    \n\
    -n    no start function                                             \n\
    -s    save asm file                                                 \n\
                                                                        \n\
    ----warnings options----                                            \n\
    --w-type    disable type convertion warnings                        \n";
    }
    else {
        
        Args[LsccArg::i] = argv[1];
        WorkingDirectory = filesystem::path(Args[LsccArg::i]).parent_path();

        size_t in1 = in("-f", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::f] = argv[in1 + 1];
        }
        else {
            if (getenv("windir") != NULL) {
                if (sizeof(void*) == 8) {
                    Args[LsccArg::f] = "win64";
                }
                else {
                    Args[LsccArg::f] = "win32";
                }
            }
            else {
                if (sizeof(void*) == 8) {
                    Args[LsccArg::f] = "elf64";
                }
                else {
                    Args[LsccArg::f] = "elf32";
                }
            }
        }

        in1 = in("-o", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::o] = argv[in1 + 1];
            afile = string(Args[LsccArg::i].substr(0, Args[LsccArg::i].rfind('.'))) + ".asm";
        }
        else {
            string filewoext = Args[LsccArg::i].substr(0, Args[LsccArg::i].rfind('.'));
            Args[LsccArg::o] = filewoext + ".obj";
            afile = filewoext + ".asm";
        }

        in1 = in("-s", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::s] = "1";
        }

        in1 = in("-cc", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::cc] = argv[in1 + 1];
        }
        else {
            if (getenv("windir") != NULL) {
                Args[LsccArg::cc] = "M64";
            }
            else {
                Args[LsccArg::cc] = "SysV";
            }
        }

        in1 = in("--w-type", argv, argc);
        if (in1 != -1) {
            Error::enabledWarns[Error::typeWarn] = false;
        }

        if (Args[LsccArg::f] == "elf64" || Args[LsccArg::f] == "win64") {
            REG.insert(REG.end(), { "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3", "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8" });
        }
        elif(Args[LsccArg::f] == "elf32" || Args[LsccArg::f] == "win32" || Args[LsccArg::f] == "elfx32") {
            REG.insert(REG.end(), { "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8"});
        }
        else {
            cout << "Error: invalid format \"" << Args[LsccArg::f] << "\" !" << endl;
            exit(1);
        }

        if (Args[LsccArg::cc] == "M64") {
            argument_order.insert(argument_order.end(), { 2, 3, 8, 9 });
        }
        elif(Args[LsccArg::cc] == "SysV") {
            argument_order.insert(argument_order.end(), { 5, 4, 3, 2, 8, 9 });
        }
        elif(Args[LsccArg::cc] == "SysVi386") {
            argument_order.clear();
        }
        else {
            cout << "Error: invalid calling convention \"" << Args[LsccArg::cc] << "\" !" << endl;
            exit(1);
        }

        compilation c;
        c = compile_file(Args[LsccArg::i], section_text, c);

        vector<token> tokens = tokenize(Args[LsccArg::i]);
        fuse_symbols(&tokens);
        identify_tokens(&tokens);
        if (in("start", get_functions_name(tokens))) {
            if (Args[LsccArg::f] == "win64" || Args[LsccArg::f] == "win32") {
                section_text += "\
GLOBAL WinMain\n\
WinMain:\n\
  call start\n\
  ret\n\
\n";
            }
        }

        for (constant& e : externs) {
            section_text += "extern " + e.name + "\n";
        }

        string code = section_text + section_data;

        //cout << code << endl;

        ofstream asFile(string(afile).c_str());

        asFile << code;

        asFile.close();

        system(string("nasm " + afile + " -f " + Args[LsccArg::f] + " -o " + Args[LsccArg::o]).c_str());

        if (Args[LsccArg::s] == "") {
            remove(afile.c_str());
        }

        cout << Error::errorTypeValid << "Successfully compiled \x1B[33m\"" << Args[LsccArg::i] << "\"\x1B[32m !" << Error::errorTypeNormal << endl;

    }

    return 0;
}