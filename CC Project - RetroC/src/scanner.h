#pragma once
#include "token.h"
#include <string>
using namespace std;

class Scanner {
private:
    string src;
    int pos;
    int line;

public:
    Scanner(string source);
    Token getNextToken();
};

