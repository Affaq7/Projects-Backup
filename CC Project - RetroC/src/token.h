#pragma once
#include <string>
using namespace std;

enum TokenType {
    T_INT, T_CHAR, T_BOOL, T_VOID,
    T_IF, T_ELSE, T_FOR, T_RETURN, T_MAIN,
    T_IDENTIFIER, T_NUMBER, T_CHAR_LITERAL,
    T_PLUS, T_MINUS, T_MUL, T_DIV,
    T_LT, T_LE, T_GT, T_GE, T_EQ, T_NE,
    T_AND, T_OR, T_NOT,
    T_ASSIGN,
    T_LPAREN, T_RPAREN,
    T_LBRACE, T_RBRACE,
    T_LBRACKET, T_RBRACKET,
    T_SEMICOLON, T_COMMA,
    T_EOF,T_TRUE,T_FALSE,T_INC,T_DEC
};

struct Token {
    TokenType type;
    string value;
    int line; // 1-based line number in the source file
};
