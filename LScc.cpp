#define _CRT_SECURE_NO_WARNINGS
#include "compilation.h"
#include "preprocessor.h"

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#else
#error "compiler must be at c++17 or above to compile!"
#endif

/*
todo:

make the language safe

arrays with basetype
arrays with datatypes
arrays with struct

lineconcat \

evaluated functions
edef foo():

*/

map < string, Basetype > type_dict = {
    { "float", Basetype::_float     },
    { "int", Basetype::_int         },
    { "str", Basetype::_string      },
    { "bool", Basetype::_bool       },
    { "ptr", Basetype::_ptr         },
    { "any", Basetype::_any         },
    { "bool", Basetype::_bool       },
    { "dbool", Basetype::_2bool     },
    { "tbool", Basetype::_3bool     },
    { "qbool", Basetype::_4bool     },
    { "qtbool", Basetype::_5bool    },
    { "hbool", Basetype::_6bool     },
    { "sbool", Basetype::_7bool     },
    { "byte", Basetype::_byte       },
    { "word", Basetype::_word       },
    { "dword", Basetype::_dword     },
    { "qword", Basetype::_qword     }
};

map < string, Tokentypes > tokens_dict = {
    { "def", Tokentypes::definition },
    { "redef", Tokentypes::redefinition },
    { "operator", Tokentypes::_operator },
    { "child", Tokentypes::child },
    { "struct", Tokentypes::_struct},
    { "mold", Tokentypes::mold_definition },
    { "end", Tokentypes::end },
    { "rel", Tokentypes::rel },
    { "$", Tokentypes::condition },
    { "[", Tokentypes::list_start },
    { "]", Tokentypes::list_end },
    { "(", Tokentypes::parameter_start },
    { ")", Tokentypes::parameter_end },
    { "{", Tokentypes::struct_start },
    { "}", Tokentypes::struct_end },
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
    { "=", Tokentypes::equal },
    { "==", Tokentypes::je },
    { "!=" , Tokentypes::jne },
    { "=True", Tokentypes::jnz },
    { "=False", Tokentypes::jz },
    { ">", Tokentypes::jg },
    { "!>", Tokentypes::jng},
    { ">=", Tokentypes::jge },
    { "!>=", Tokentypes::jnge },
    { "<", Tokentypes::jl },
    { "!<", Tokentypes::jnl },
    { "<=", Tokentypes::jle },
    { "!<=", Tokentypes::jnle },
    { "is", Tokentypes::is },
    { "$!", Tokentypes::inv }
};

int ROspaces = 0;
string section_data = "SECTION .data:\n";
string section_text = "SECTION .text:\n";
vector<int> argument_order;
vector<int> fargument_order = { 16, 17, 18, 19, 20, 21, 22, 23, 24 };
vector<constant> spaces;
vector<asoperator> operators;
vector<structure> structs;
vector<aschild> childs;
vector<constant> externs;
vector<string> pcllibs, includes_f, REG;
CallingConvention convention;
preprocessor preproc;
int architecture = 64;

map<LsccArg, string> Args {
    {LsccArg::cc, ""},
    {LsccArg::f, ""},
    {LsccArg::i, ""},
    {LsccArg::n, ""},
    {LsccArg::none, ""},
    {LsccArg::o, ""},
    {LsccArg::p, ""},
    {LsccArg::s, ""},
    {LsccArg::info, ""}
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
    -cc   calling convention[SysV, ABI, M64, cdecl]                 \n\
    -f    format[elf32, elf64, elfx32, win32, win64]                    \n\
    -n    no start function                                             \n\
    -s    save asm file                                                 \n\
    -info shows basic infos                                             \n\
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

        in1 = in("-info", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::info] = "1";
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

        in1 = in("-p", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::p] = "1";
        }
        
        in1 = in("-cc", argv, argc);
        if (in1 != -1) {
            Args[LsccArg::cc] = argv[in1 + 1];
        }
        else {
            Args[LsccArg::cc] = "cdecl";
        }

        in1 = in("--w-type", argv, argc);
        if (in1 != -1) {
            Error::enabledWarns[Error::typeWarn] = false;
        }

        if (Args[LsccArg::f] == "elf64" || Args[LsccArg::f] == "win64") {
            architecture = 64;
            REG.insert(REG.end(), { "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3", "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7" });
        }
        elif(Args[LsccArg::f] == "elf32" || Args[LsccArg::f] == "win32" || Args[LsccArg::f] == "elfx32") {
            architecture = 32; 
            REG.insert(REG.end(), { "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "cr0", "cr2", "cr3", "cr4", "cr8" });
        }
        else {
            cout << "Error: invalid format \"" << Args[LsccArg::f] << "\" !" << endl;
            std::exit(1);
        }

        if (Args[LsccArg::cc] == "M64") {
            argument_order.insert(argument_order.end(), { 2, 3, 8, 9 });
            convention = CallingConvention::M64;
        }
        elif(Args[LsccArg::cc] == "ABI") {
            argument_order.insert(argument_order.end(), { 5, 4, 3, 2, 8, 9 });
            convention = CallingConvention::ABI;
        }
        elif(Args[LsccArg::cc] == "SysV") {
            argument_order.clear();
            convention = CallingConvention::SysV;
        }
        elif(Args[LsccArg::cc] == "cdecl") {
            argument_order.insert(argument_order.end(), { 2, 3, 8, 9 });
            convention = CallingConvention::___cdecl;
        }
        else {
            cout << "Error: invalid calling convention \"" << Args[LsccArg::cc] << "\" !" << endl;
            std::exit(1);
        }

        compilation c;
        c = compile_file(Args[LsccArg::i], section_text, c);

        vector<token> tokens = tokenize(Args[LsccArg::i]);
        fuse_symbols(&tokens);
        identify_tokens(&tokens);

        for (constant& e : externs) {
            section_text += "extern " + e.name + "\n";
        }

        string code = section_text + section_data;

        if (Args[LsccArg::p] == "1") {
            cout << code << endl;
        }

        ifstream file(Args[LsccArg::i]);
        ostringstream originalContent;
        originalContent << file.rdbuf();
        file.close();

        ofstream asFile(Args[LsccArg::i]);
        asFile << code;
        asFile.close();

        int nasm_success = system(string("nasm " + Args[LsccArg::i] + " -f " + Args[LsccArg::f] + " -o " + Args[LsccArg::o]).c_str());

        asFile.open(Args[LsccArg::i]);
        asFile << originalContent.str();
        asFile.close();

        if (Args[LsccArg::s] == "1") {
            asFile.open(afile);
            asFile << code;
            asFile.close();
        }

        if (nasm_success == 0) {
            cout << Error::errorTypeValid << "Successfully compiled \x1B[33m\"" << Args[LsccArg::i] << "\"\x1B[32m !" << Error::errorTypeNormal << endl;
        }
        else {
            cout << Error::errorTypeError << "UnSuccessfully compiled \x1B[33m\"" << Args[LsccArg::i] << "\"\x1B[31m !" << Error::errorTypeNormal << endl;
        }
    }

    return 0;
}