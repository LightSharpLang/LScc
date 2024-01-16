#include "preprocessor.h"


void preprocessor::processFile(int f, vector<token> _tokens) {
    string name = "";
    vector<token> tokens;

    mold m;
    lschild c;
    lsoperator o;

    vector<constant> constants = {};

    // getting the function's tokens
    for (int i = f; i < _tokens.size(); i++) {
        if (_tokens[i].type != Tokentypes::end) {
            tokens.push_back(_tokens[i]);
        }
        else {
            break;
        }
    }

    bool isMold = false;
    bool isChild = false;
    bool isOperator = false;

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
                isMold = true;
                break;
            case Tokentypes::child:
                isChild = true;
                break;
            case Tokentypes::_operator:
                isOperator = true;
                break;
            }
        }
        if (tokens[i].type == Tokentypes::_constant && tokens[i + 1].type == Tokentypes::parameter_start) {
            name = tokens[i].t;
        }
        if (tokens[i].type == Tokentypes::_constant && parenthesis) {
            if (tokens[i - 1].type == Tokentypes::type) {
                constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", getType(tokens[i - 1])));
            }
            else {
                constants.push_back(constant(tokens[i].t, Tokentypes::variable, "[" + REG[6] + " - " + to_string(8 * (i + 1)) + "]", Basetype::_any));
            }
        }
    }

    if (name == "") {
        cerr << Error::errorTypeError << "Error: Function has no name at line " << Error::errorTypeNormal << tokens[0].line << " maybe you forgot `('?\n";
    }

    if (isMold) {
        m.name = name;
        for (auto c : constants) {
            m.parameters.push_back(c);
        }
        m.code = tokens;
        m.offset = f;
        molds.push_back(m);
    }
    elif(isChild) {
        c.name = name;
        for (auto c2 : constants) {
            c.parameters.push_back(c2);
        }
        c.code = tokens;
        c.offset = f;
        childs.push_back(c);
    }
    elif(isOperator) {
        o.name = name;
        for (auto c : constants) {
            o.parameters.push_back(c);
        }
        o.code = tokens;
        o.offset = f;
        operators.push_back(o);
    }
}