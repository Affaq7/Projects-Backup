#include "scanner.h"  // Include declarations for Scanner and Token types
#include <cctype>      // For character classification functions like isspace, isalpha, isdigit
using namespace std;   // Use the standard namespace

Scanner::Scanner(string source)  // Constructor initializes scanner with source code
{
    src = source;  // Store the source string
    pos = 0;       // Start at the beginning of the source
    line = 1;      // Line numbers start at 1
}

Token Scanner::getNextToken()  // Lex the next token from the source
{
    // Skip whitespace and comments
    while (pos < src.size())  // Loop while there is input left
    {
        // Skip whitespace
        if (isspace(src[pos]))  // Check if current char is whitespace
        {
            if (src[pos] == '\n')  // Track line breaks
                line++;
            pos++;  // Advance past whitespace
            continue;  // Continue skipping
        }

        // Skip single-line comment: //
        if (src[pos] == '/' && pos + 1 < src.size() && src[pos + 1] == '/')  // Detect //
        {
            pos += 2; // skip //  // Move past //
            while (pos < src.size() && src[pos] != '\n')  // Consume until end of line
                pos++;
            continue; // restart loop  // Continue scanning after comment
        }

        // Skip multi-line comment: /* ... */
        if (src[pos] == '/' && pos + 1 < src.size() && src[pos + 1] == '*')  // Detect /*
        {
            pos += 2; // skip /*  // Move past /*
            while (pos + 1 < src.size())  // Ensure we can look ahead for */
            {
                if (src[pos] == '\n')  // Count newlines inside comment
                    line++;

                // End of comment */
                if (src[pos] == '*' && src[pos + 1] == '/')  // Detect */
                {
                    pos += 2;  // Move past */
                    break;     // Stop skipping comment
                }
                pos++;  // Advance within comment
            }
            continue;  // Resume scanning after comment
        }

        // If none matched no more whitespace/comments
        break;  // Exit skipping loop
    }

    if (pos >= src.size())  // If at end of input
        return {T_EOF, "", line};  // Return EOF token

    char c = src[pos];  // Current character

    // Identifiers & Keywords
    if (isalpha(c) || c == '_')  // Start of identifier or keyword
    {
        string word = "";  // Accumulate lexeme
        while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_'))  // Continue over letters/digits/underscore
        {
            word += src[pos++];  // Append and advance
        }
        if (word == "int")  // Keyword: int
            return {T_INT, word, line};
        if (word == "char")  // Keyword: char
            return {T_CHAR, word, line};
        if (word == "bool")  // Keyword: bool
            return {T_BOOL, word, line};
        if (word == "void")  // Keyword: void
            return {T_VOID, word, line};

        if (word == "if")  // Keyword: if
            return {T_IF, word, line};
        if (word == "else")  // Keyword: else
            return {T_ELSE, word, line};
        if (word == "for")  // Keyword: for
            return {T_FOR, word, line};
        if (word == "return")  // Keyword: return
            return {T_RETURN, word, line};
        if (word == "main")  // Keyword treated specially: main
            return {T_MAIN, word, line};
        if (word == "true")  // Keyword: true
            return {T_TRUE, word, line};   // ADD THIS
        if (word == "false")  // Keyword: false
            return {T_FALSE, word, line}; // ADD THIS
        return {T_IDENTIFIER, word, line};  // Otherwise, an identifier
    }
    if (c == ',')  // Comma
    {
        pos++; // move past ','  // Advance position
        return {T_COMMA, ",", line};  // Return comma token
    }
    if (c == '&' && pos + 1 < src.size() && src[pos + 1] == '&')  // Logical AND &&
    {
        pos += 2;  // Consume both characters
        return {T_AND, "&&", line};  // Return AND token
    }
    if (c == '|' && pos + 1 < src.size() && src[pos + 1] == '|')  // Logical OR ||
    {
        pos += 2;  // Consume both characters
        return {T_OR, "||", line};  // Return OR token
    }
    // Check for multi-char operators first
    if (c == '=')  // Assignment or equality
    {
        if (pos + 1 < src.size() && src[pos + 1] == '=')  // ==
        {
            pos += 2;  // Consume ==
            return {T_EQ, "==", line};  // Equality token
        }
        else
        {
            pos++;  // Consume =
            return {T_ASSIGN, "=", line};  // Assignment token
        }
    }
    if (c == '!')  // Not or not-equal
    {
        if (pos + 1 < src.size() && src[pos + 1] == '=')  // !=
        {
            pos += 2;  // Consume !=
            return {T_NE, "!=", line};  // Not-equal token
        }
        else
        {
            pos++;  // Consume !
            return {T_NOT, "!", line};  // Logical NOT token
        }
    }
    if (c == '<')  // Less-than or <=
    {
        if (pos + 1 < src.size() && src[pos + 1] == '=')  // <=
        {
            pos += 2;  // Consume <=
            return {T_LE, "<=", line};  // Less-or-equal token
        }
        else
        {
            pos++;  // Consume <
            return {T_LT, "<", line};  // Less-than token
        }
    }
    if (c == '>')  // Greater-than or >=
    {
        if (pos + 1 < src.size() && src[pos + 1] == '=')  // >=
        {
            pos += 2;  // Consume >=
            return {T_GE, ">=", line};  // Greater-or-equal token
        }
        else
        {
            pos++;  // Consume >
            return {T_GT, ">", line};  // Greater-than token
        }
    }

    // Character literals
    if (c == '\'')  // Start of character literal
    {
        pos++; // skip opening quote  // Move past '
        if (pos < src.size())  // Ensure there is a char
        {
            char charVal = src[pos++];  // Capture the character and advance
            if (pos < src.size() && src[pos] == '\'')  // Expect closing '
            {
                pos++; // skip closing quote  // Consume '
                return {T_CHAR_LITERAL, string(1, charVal), line};  // Return char literal token
            }
            else
            {
                return {T_EOF, ""}; // error: unterminated character literal  // Sentinel or error handling
            }
        }
    }

    // Numbers
    if (isdigit(c))  // Start of numeric literal
    {
        string num = "";  // Accumulate digits
        while (pos < src.size() && isdigit(src[pos]))  // Read consecutive digits
        {
            num += src[pos++];  // Append and advance
        }
        return {T_NUMBER, num, line};  // Return number token
    }

    // Handle operators
    pos++;  // For single-char tokens, advance one char
    
    switch (c)
    {
    case '+':
        // Check for ++
        if (pos < src.size() && src[pos] == '+') {
            pos++;  // consume second +
            return {T_INC, "++", line};  // NEW
        }
        return {T_PLUS, "+", line};
    
    case '-':
        // Check for --
        if (pos < src.size() && src[pos] == '-') {
            pos++;  // consume second -
            return {T_DEC, "--", line};  // NEW
        }
        return {T_MINUS, "-", line};
    
    case '*':
        return {T_MUL, "*", line};
    
    case '/':
        return {T_DIV, "/", line};
    
    case '[':  // Left bracket
        return {T_LBRACKET, "[", line};
    case ']':  // Right bracket
        return {T_RBRACKET, "]", line};
    case '(':  // Left parenthesis
        return {T_LPAREN, "(", line};
    case ')':  // Right parenthesis
        return {T_RPAREN, ")", line};
    case '{':  // Left brace
        return {T_LBRACE, "{", line};
    case '}':  // Right brace
        return {T_RBRACE, "}", line};
    case ';':  // Semicolon
        return {T_SEMICOLON, ";", line};
    default:  // Unknown character
        return {T_EOF, "", line};  // Fallback token; consider an error type instead
    }
}
