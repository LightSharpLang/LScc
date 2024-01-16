#pragma once
#include "token.h"
#include "compilation.h"

class mold {
public:
    std::string name;
    vector<constant> parameters;
    vector<token> code;
    Basetype type;
    int offset;
};

struct lsoperator {
public:
    std::string name;
    vector<constant> parameters;
    vector<token> code;
    Basetype type;
    int offset;
};

struct lschild {
public:
    std::string name;
    vector<constant> parameters;
    vector<token> code;
    Basetype type;
    int offset;
};

class preprocessor {
public:
	vector<mold> molds;
	vector<lsoperator> operators;
	vector<lschild> childs;
    void processFile(int f, vector<token> tokens);
};