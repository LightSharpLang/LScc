#include "token.h"
#include <fstream>

token::token(string str) {
    this->t = str;
}

bool in(string a, string b[], int len) {
    for (int i = 0; i < len; i++) {
        if (a == b[i]) return true;
    }
    return false;
}

size_t in(string a, char** b, int len) {
    for (int i = 0; i < len; i++) {
        if (a == b[i]) return i;
    }
    return -1;
}

size_t in(string a, vector<coperator> b) {
    for (size_t i = 0; i < b.size(); i++) {
        if (a == b[i].name) return true;
    }
    return false;
}

bool alnum(string str) {
    string valid_chars = "azertyuiopqsdfghjklmwxcvbnAZERTYUIOPQSDFGHJKLMWXCVBN1234567890";
    for (char c : str) {
        if (valid_chars.find(c) == string::npos) return false;
    }
    return true;
}

void identify_tokens(vector<token>* tokens, bool is_lib) {
    int line = 1;
    bool nowarn = false;
    bool is_comment = false;
    if (tokens->at(0).t == "nowarn") nowarn = true;     
    for (int i = 0; i < tokens->size(); i++) {
        tokens->at(i).line = line;
        if (tokens->at(i).t == "\n") {
            line += 1;
            is_comment = false;
        }
        const auto it = tokens_dict.find(tokens->at(i).t);
        if (is_comment) {
            tokens->at(i).type == Tokentypes::comment;
        }
        elif (it != tokens_dict.end()) {
            // token in token_dict
            tokens->at(i).type = it->second;
            tokens->at(i).identified = true;
            if (tokens->at(i).type == Tokentypes::comment) is_comment = true;
        }
        elif(in(tokens->at(i).t, operators)) {
            // token in symbols
            tokens->at(i).type = Tokentypes::_operator;
            tokens->at(i).identified = true;
        }
        elif(alnum(tokens->at(i).t)) tokens->at(i).type = Tokentypes::_constant;
        elif(tokens->at(i).t[0] == '"') tokens->at(i).type = Tokentypes::litteral;
        elif(tokens->at(i).t[0] == '\'') tokens->at(i).type = Tokentypes::litteral;
        else {
            if (is_lib) {
                if (!nowarn)
                cout << Error::errorTypeWarn << "warning:" << Error::errorTypeNormal << " syntax: '" << tokens->at(i).t << "' at line " << tokens->at(i).line << " may not be right" << endl;
            }
            else {
                cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << " at line " << tokens->at(i).line << ", invalid syntax: '" << tokens->at(i).t << endl;
                exit(1);
            }
        }
    }
}

void fuse_symbols(vector<token>* tokens) {
    int fused = 0;
    size_t size = tokens->size();
    for (size_t j = 0; j < size; j++) {
        fused = 0;
        size = tokens->size();
        for (size_t i = 0; i < size - 1; i++) {
                if (tokens_dict.find(tokens->at(i - fused).t + tokens->at(i - fused + 1).t) != tokens_dict.end()) {
                    tokens->at(i - fused).t += tokens->at(i - fused + 1).t;
                    tokens->erase((tokens->begin() + (i - fused)) + 1);
                    fused += 1;
                }
        }
    }
}

void fuse_string_litterals(vector<token>* tokens) {
    int fused = 0;
    bool is_litteral = 0;
    int litteral_start = 0;
    int size = tokens->size();
    for (int i = 0; i < size; i++) {
        if (tokens->at(i - fused).t == "\"" && !is_litteral) {
            litteral_start = i - fused;
            is_litteral = 1;
            continue;
        }
        if (tokens->at(i - fused).t == "\"" && is_litteral) {
            is_litteral = 0;
            string litteral = "";
            int fused_tokens = 0;
            for (int j = litteral_start; j < i - fused + 1; j++) {
                litteral += tokens->at(j).t;
            }
            token t = token(litteral);
            for (int j = litteral_start; j < i - fused + 1; j++) {
                tokens->erase(tokens->begin() + litteral_start);
                fused_tokens += 1;
            }
            fused += fused_tokens;
            tokens->insert(tokens->begin() + litteral_start - 1, t);
            litteral_start = 0;
        }
    }
    fused = 0;
    is_litteral = 0;
    litteral_start = 0;
    for (int i = 0; i < tokens->size(); i++) {
        if (tokens->at(i - fused).t == "\'" && !is_litteral) {
            litteral_start = i - fused;
            is_litteral = 1;
            continue;
        }
        if (tokens->at(i - fused).t == "\'" && is_litteral) {
            is_litteral = 0;
            string litteral = "";
            int fused_tokens = 0;
            for (int j = litteral_start; j < i - fused + 1; j++) {
                litteral += tokens->at(j).t;

            }
            token t = token(litteral);
            for (int j = litteral_start; j < i + 1; j++) {
                tokens->erase(tokens->begin() + litteral_start);
                fused_tokens += 1;
            }
            fused += fused_tokens;
            tokens->insert(tokens->begin() + litteral_start - 1, t);
        }
    }
}

void clean_tokens(vector<token>* tokens) {
    int poped = 0;
    size_t s = tokens->size();
    for (int i = 0; i < s; i++) {
        if (tokens->at(i - poped).t == "" || tokens->at(i - poped).t[0] == ' ') {
            tokens->erase(tokens->begin() + (i - poped));
            poped += 1;
        }
    }
}

vector<token> tokenize(string file, bool isstr) {
    vector<token> tokens = {};
    string valid_chars = "_azertyuiopqsdfghjklmwxcvbnAZERTYUIOPQSDFGHJKLMWXCVBN1234567890";
    char separate_token = ' ';
    string line;
    if (!isstr) {
        ifstream myfile(file.c_str());

        if (!myfile.good()) {
            cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << ", file  \"" << file << "\" does not exist!" << endl;
            exit(1);
        }
        if (myfile.is_open())
        {
            string current_token = "";
            while (getline(myfile, line))
            {
                for (char chr : line) {
                    const auto it = valid_chars.find(chr);
                    // if alphanums
                    if (chr != separate_token && it != string::npos) {
                        current_token += chr;
                    }
                    else {
                        // if not alphanum
                        if (it == string::npos) {
                            tokens.push_back(token(current_token));
                            string c = "a";
                            c[0] = chr;
                            tokens.push_back(token(c));
                            current_token = "";
                        }
                        // if space
                        else {
                            tokens.push_back(token(current_token));
                            tokens.push_back(token(string(" ")));
                            current_token = "";
                        }
                    }
                }
                tokens.push_back(current_token);
                tokens.push_back(token("\n"));
                current_token = "";
            }
            myfile.close();
        }
        else {
            cout << Error::errorTypeError << "Error" << Error::errorTypeNormal << ", cannot read file \"" << file << "\" !" << endl;
            exit(1);
        }
    }
    else {
        size_t i = 0;
        string current_token = "";
        while (file[i] != 0)
        {
            for (char chr : file) {
                i++;
                const auto it = valid_chars.find(chr);
                // if alphanums
                if (chr != separate_token && it != string::npos) {
                    current_token += chr;
                }
                else {
                    // if not alphanum
                    if (it == string::npos) {
                        tokens.push_back(token(current_token));
                        string c = "a";
                        c[0] = chr;
                        tokens.push_back(token(c));
                        current_token = "";
                    }
                    // if space
                    else {
                        tokens.push_back(token(current_token));
                        tokens.push_back(token(string(" ")));
                        current_token = "";
                    }
                }
            }
            tokens.push_back(current_token);
            current_token = "";
        }
    }
    fuse_string_litterals(&tokens);
    fuse_symbols(&tokens);
    clean_tokens(&tokens);
    return tokens;
}

string coperator::fcode(int ident) {
    vector<token> tokens = tokenize(code, true);
    identify_tokens(&tokens);

    stringstream c;

    Tokentypes prev = Tokentypes::lf;
    for (token t : tokens) {
        if (prev == Tokentypes::lf) {
            c << string(ident, ' ') << t.t;
        }
        else {
            c << " " << t.t;
        }
        prev = t.type;
    }
    
    return c.str();
}