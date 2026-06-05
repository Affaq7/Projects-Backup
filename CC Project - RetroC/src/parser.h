#pragma once
#include "scanner.h"
#include "semantic.h"
#include "ir.h"
using namespace std;

// Return type for expression-parsing methods:
// carries both the semantic type string and the TAC "place" (temp or variable name)
struct ExprResult {
    string type;   // "int", "bool", "char", ""
    string place;  // TAC operand: temp name, variable name, or literal string
};

class Parser {
private:
    Scanner      scanner;    // Lexical scanner
    Token        current;    // Current token being processed
    Semantic     semantic;   // Semantic analyser
    IRGenerator  irgen;      // TAC generator (Phase 4)
    string       fileName;   // Source file name

    void advance();
    void match(TokenType t);
    bool check(TokenType t);
    bool isType(TokenType t);
    void skipToRecovery();

    void program();
    void declaration();
    void statement();
    void block();

    // Function parsing
    void parameterList();
    void argumentList();
    TokenType parseType();

    // Control flow
    void ifStatement();
    void forStatement();
    void returnStatement();

    // Expression parsing — each returns ExprResult{type, place}
    ExprResult expression();
    ExprResult parseLogical();
    ExprResult parseRelational();
    ExprResult parseTerm();
    ExprResult parseFactor();
    ExprResult parsePrimary();

public:
    Parser(Scanner sc, const string& fileName);
    void parse();

    // Expose the generated IR so main() can access it
    IRGenerator& getIR() { return irgen; }
};
