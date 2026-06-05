#include "parser.h"
#include <iostream>
#include <cstdlib>
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Parser::Parser(Scanner sc, const string& fileName)
    : scanner(sc), semantic(fileName), fileName(fileName)
{
    current = scanner.getNextToken();
}

// ─────────────────────────────────────────────────────────────────────────────
// Token helpers
// ─────────────────────────────────────────────────────────────────────────────
void Parser::advance()          { current = scanner.getNextToken(); }
bool Parser::check(TokenType t) { return current.type == t; }
bool Parser::isType(TokenType t){
    return t==T_INT||t==T_CHAR||t==T_BOOL||t==T_VOID;
}
void Parser::match(TokenType t) {
    if (current.type == t) advance();
    else cout << "Syntax Error! Expected token " << (int)t
              << " but got '" << current.value << "' at "
              << fileName << ":" << current.line << "\n";
}
void Parser::skipToRecovery() {
    while (current.type!=T_SEMICOLON && current.type!=T_LBRACE &&
           current.type!=T_RBRACE   && current.type!=T_EOF     &&
           !isType(current.type))
        advance();
}

// Converts type token to string
static string tokToTypeStr(TokenType t){
    if(t==T_INT)  return "int";
    if(t==T_CHAR) return "char";
    if(t==T_BOOL) return "bool";
    return "void";
}

// ─────────────────────────────────────────────────────────────────────────────
// Public entry point
// ─────────────────────────────────────────────────────────────────────────────
void Parser::parse() { program(); semantic.finalize(); }

// ─────────────────────────────────────────────────────────────────────────────
// Program — top-level declarations and function definitions
// ─────────────────────────────────────────────────────────────────────────────
void Parser::program() {
    semantic.enterScope();
    while (current.type != T_EOF) {
        if (isType(current.type)) {
            TokenType type    = current.type;
            int       typeLine = current.line;
            advance();

            if (current.type == T_IDENTIFIER || current.type == T_MAIN) {
                string name     = (current.type==T_MAIN) ? "main" : current.value;
                int    nameLine = current.line;
                advance();

                if (current.type == T_LPAREN) {
                    // ── Function declaration / definition ──────────────────
                    advance(); // skip '('
                    vector<string> paramTypes, paramNames;
                    if (isType(current.type)) {
                        while (true) {
                            TokenType pt = current.type;
                            advance();
                            if (current.type != T_IDENTIFIER) {
                                cout<<"Syntax Error: expected param name at "
                                    <<fileName<<":"<<current.line<<"\n";
                                skipToRecovery(); break;
                            }
                            paramTypes.push_back(tokToTypeStr(pt));
                            paramNames.push_back(current.value);
                            advance();
                            if (current.type==T_COMMA){ advance();
                                if(!isType(current.type)) break;
                                continue; }
                            break;
                        }
                    }
                    if (current.type==T_RPAREN) advance();
                    else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";

                    string retType = tokToTypeStr(type);

                    if (current.type == T_SEMICOLON) {
                        // Forward declaration — no IR body
                        semantic.declareFunction(name,retType,paramTypes,paramNames,nameLine,false);
                        advance();
                    } else if (current.type == T_LBRACE) {
                        // Definition
                        if (semantic.declareFunction(name,retType,paramTypes,paramNames,nameLine,true)) {
                            // Build param signature string for IR header
                            string sig;
                            for(size_t i=0;i<paramTypes.size();i++){
                                if(i) sig+=",";
                                sig+=paramTypes[i]+" "+paramNames[i];
                            }
                            irgen.emit(IR_FUNC_BEGIN, name, sig);
                            semantic.enterFunctionBody(name);
                            // Declare parameters in IR + semantic
                            for (size_t i=0;i<paramNames.size();i++){
                                semantic.declareVariable(paramNames[i],paramTypes[i],false,-1,nameLine,"parameter");
                                semantic.noteInitialization(paramNames[i],nameLine);
                                irgen.emit(IR_DECLARE, paramNames[i], paramTypes[i]);
                            }
                            block();
                            semantic.leaveFunctionBody();
                            irgen.emit(IR_FUNC_END, name);
                        } else {
                            block(); // skip duplicate body
                        }
                    } else {
                        cout<<"Syntax Error: expected ';' or '{' after function signature at "
                            <<fileName<<":"<<current.line<<"\n";
                        skipToRecovery();
                        if(current.type==T_SEMICOLON) advance();
                        else if(current.type==T_LBRACE) block();
                    }
                }
                else if (current.type==T_LBRACKET || current.type==T_ASSIGN ||
                         current.type==T_COMMA    || current.type==T_SEMICOLON) {
                    // ── Global variable declaration ────────────────────────
                    bool isArray=false; int arrSize=-1;
                    if (current.type==T_LBRACKET){
                        advance();
                        if(current.type==T_NUMBER){arrSize=stoi(current.value);advance();}
                        if(current.type==T_RBRACKET) advance();
                        isArray=true;
                    }
                    string baseType = tokToTypeStr(type);
                    bool ok = semantic.declareVariable(name,baseType,isArray,arrSize,nameLine,"variable");
                    irgen.emit(IR_DECLARE, name, isArray?(baseType+"["+to_string(arrSize)+"]"):baseType);
                    string initRepr;
                    if (current.type==T_ASSIGN){
                        advance();
                        initRepr="initializer";
                        ExprResult rhs = expression();
                        if(ok){ semantic.checkAssignmentType(name,rhs.type,nameLine);
                                semantic.noteInitialization(name,nameLine);
                                irgen.emit(IR_ASSIGN,name,rhs.place); }
                    }
                    if(ok){
                        string tr=isArray?baseType+"["+to_string(arrSize)+"]":baseType;
                        semantic.logDeclarationParsed(name,tr,nameLine,initRepr);
                    }
                    if(current.type==T_SEMICOLON) advance();
                    else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";
                } else {
                    cout<<"Syntax Error: unexpected token after identifier at "
                        <<fileName<<":"<<current.line<<"\n";
                    skipToRecovery();
                    if(current.type==T_SEMICOLON) advance();
                }
            } else {
                cout<<"Syntax Error: expected identifier after type at "
                    <<fileName<<":"<<current.line<<"\n";
                skipToRecovery();
            }
        } else if (current.type==T_SEMICOLON) {
            advance();
        } else {
            statement();
        }
    }
    semantic.leaveScope();
}

// ─────────────────────────────────────────────────────────────────────────────
// Block  { statements }
// ─────────────────────────────────────────────────────────────────────────────
void Parser::block() {
    if(current.type==T_LBRACE) advance();
    else cout<<"Syntax Error: expected '{' at "<<fileName<<":"<<current.line<<"\n";

    semantic.enterScope();
    while(current.type!=T_RBRACE && current.type!=T_EOF){
        if(isType(current.type)) declaration();
        else                      statement();
    }
    if(current.type==T_RBRACE) advance();
    else cout<<"Syntax Error: expected '}' at "<<fileName<<":"<<current.line<<"\n";
    semantic.leaveScope();
}

// ─────────────────────────────────────────────────────────────────────────────
// Declaration  type id [= expr] [, id ...] ;
// ─────────────────────────────────────────────────────────────────────────────
void Parser::declaration() {
    TokenType type = current.type;
    advance();

    while(true){
        if(current.type!=T_IDENTIFIER){
            cout<<"Syntax Error: expected identifier at "<<fileName<<":"<<current.line<<"\n";
            skipToRecovery(); break;
        }
        string varName  = current.value;
        int    nameLine = current.line;
        advance();

        bool isArray=false; int arrSize=-1;
        if(current.type==T_LBRACKET){
            advance();
            if(current.type==T_NUMBER){ arrSize=stoi(current.value); advance(); }
            if(current.type==T_RBRACKET) advance();
            else cout<<"Syntax Error: expected ']' at "<<fileName<<":"<<current.line<<"\n";
            isArray=true;
        }

        string baseType = tokToTypeStr(type);
        bool ok = semantic.declareVariable(varName,baseType,isArray,arrSize,nameLine,"variable");
        string typeRepr = isArray ? baseType+"["+to_string(arrSize)+"]" : baseType;
        irgen.emit(IR_DECLARE, varName, typeRepr);

        string initRepr;
        if(current.type==T_ASSIGN){
            advance();
            initRepr="initializer";
            ExprResult rhs = expression();
            if(ok){
                semantic.checkAssignmentType(varName,rhs.type,nameLine);
                semantic.noteInitialization(varName,nameLine);
                irgen.emit(IR_ASSIGN, varName, rhs.place);
            }
        }
        if(ok) semantic.logDeclarationParsed(varName,typeRepr,nameLine,initRepr);

        if(current.type==T_COMMA){ advance(); continue; }
        if(current.type==T_SEMICOLON){ advance(); break; }
        cout<<"Syntax Error: expected ',' or ';' at "<<fileName<<":"<<current.line<<"\n";
        skipToRecovery(); break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Statement
// ─────────────────────────────────────────────────────────────────────────────
void Parser::statement() {
    if(current.type==T_IF)     { ifStatement();     return; }
    if(current.type==T_FOR)    { forStatement();    return; }
    if(current.type==T_RETURN) { returnStatement(); return; }
    if(current.type==T_LBRACE) { block();           return; }

    if(current.type==T_IDENTIFIER){
        string varName  = current.value;
        int    nameLine = current.line;
        advance();

        if(current.type==T_LBRACKET){
            // Array element assignment:  arr[idx] = expr;
            advance();
            string idxPlace;
            string idxType;
            if(current.type==T_NUMBER){
                int idx=stoi(current.value);
                semantic.checkArrayIndex(varName,idx,current.line);
                idxPlace=to_string(idx);
                idxType="int";
                advance();
            } else {
                ExprResult er = expression();
                idxPlace = er.place;
                idxType  = er.type;
                semantic.checkArrayIndexType(idxType,current.line);
            }
            if(current.type==T_RBRACKET) advance();
            else cout<<"Syntax Error: expected ']' at "<<fileName<<":"<<current.line<<"\n";

            if(current.type==T_ASSIGN){
                advance();
                ExprResult rhs = expression();
                semantic.checkArrayElementAssignment(varName,rhs.type,nameLine);
                semantic.noteInitialization(varName,nameLine);
                irgen.emit(IR_ARRAY_STORE, varName, rhs.place, idxPlace);
                semantic.logAssignment(varName+"[index]","char",nameLine);
                if(current.type==T_SEMICOLON) advance();
                else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";
            } else {
                cout<<"Syntax Error: expected '=' after array access at "
                    <<fileName<<":"<<current.line<<"\n";
                skipToRecovery();
            }
        }
        else if(current.type==T_LPAREN){
            // Function call statement:  f(args);
            advance();
            vector<string> argTypes;
            vector<string> argPlaces;
            if(current.type!=T_RPAREN){
                ExprResult a = expression();
                argTypes.push_back(a.type);
                argPlaces.push_back(a.place);
                while(current.type==T_COMMA){
                    advance();
                    ExprResult b = expression();
                    argTypes.push_back(b.type);
                    argPlaces.push_back(b.place);
                }
            }
            if(current.type==T_RPAREN) advance();
            else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";

            semantic.logFunctionCall(varName,nameLine);
            semantic.checkFunctionCall(varName,argTypes,nameLine);
            for(const auto& p : argPlaces) irgen.emit(IR_PARAM,"",p);
            irgen.emit(IR_CALL, "", varName, to_string(argPlaces.size()));

            if(current.type==T_SEMICOLON) advance();
            else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";
        }
        else if(current.type==T_ASSIGN){
            // Variable assignment:  x = expr;
            advance();
            ExprResult rhs = expression();
            semantic.checkAssignmentType(varName,rhs.type,nameLine);
            semantic.noteInitialization(varName,nameLine);
            irgen.emit(IR_ASSIGN, varName, rhs.place);
            semantic.logAssignment(varName,"expression",nameLine);
            if(current.type==T_SEMICOLON) advance();
            else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";
        }
        else {
            cout<<"Syntax Error: expected '=', '[', or '(' after identifier at "
                <<fileName<<":"<<current.line<<"\n";
            skipToRecovery();
        }
        return;
    }

    if(current.type==T_SEMICOLON){ advance(); return; } // empty statement

    cout<<"Syntax Error: unexpected token '"<<current.value<<"' in statement at "
        <<fileName<<":"<<current.line<<"\n";
    skipToRecovery();
}

// ─────────────────────────────────────────────────────────────────────────────
// If statement
// ─────────────────────────────────────────────────────────────────────────────
void Parser::ifStatement() {
    advance(); // 'if'
    if(current.type==T_LPAREN) advance();
    else cout<<"Syntax Error: expected '(' at "<<fileName<<":"<<current.line<<"\n";

    ExprResult cond = expression();

    if(current.type==T_RPAREN) advance();
    else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";

    string labelElse = irgen.newLabel();
    string labelEnd  = irgen.newLabel();
    irgen.emit(IR_IF_FALSE, labelElse, cond.place);

    if(current.type==T_LBRACE) block(); else statement();

    if(current.type==T_ELSE){
        advance();
        irgen.emit(IR_GOTO,  labelEnd);
        irgen.emit(IR_LABEL, labelElse);
        if(current.type==T_LBRACE) block(); else statement();
        irgen.emit(IR_LABEL, labelEnd);
    } else {
        irgen.emit(IR_LABEL, labelElse);
    }
    semantic.logIfStatement(current.line);
}

// ─────────────────────────────────────────────────────────────────────────────
// For loop
// ─────────────────────────────────────────────────────────────────────────────
void Parser::forStatement() {
    advance(); // 'for'
    if(current.type==T_LPAREN) advance();
    else cout<<"Syntax Error: expected '(' at "<<fileName<<":"<<current.line<<"\n";

    // Init
    if(current.type==T_SEMICOLON) advance();
    else if(isType(current.type)) declaration();
    else { expression(); if(current.type==T_SEMICOLON) advance(); }

    string labelStart = irgen.newLabel();
    string labelEnd   = irgen.newLabel();
    irgen.emit(IR_LABEL, labelStart);

    // Condition
    if(current.type!=T_SEMICOLON){
        ExprResult cond = expression();
        if(cond.type!="bool")
            semantic.logError("For-loop condition must be bool, not "+cond.type, current.line);
        irgen.emit(IR_IF_FALSE, labelEnd, cond.place);
    }
    if(current.type==T_SEMICOLON) advance();
    else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";

    // Collect increment code — emit it after body
    // We parse increment but buffer its instructions
    int incStart = (int)irgen.getInstructions().size();
    if(current.type!=T_RPAREN){
        if(current.type==T_IDENTIFIER){
            string idName = current.value;
            int    idLine = current.line;
            advance();
            if(current.type==T_INC){
                advance();
                semantic.noteUse(idName,idLine,"increment");
                string tmp = irgen.newTemp();
                irgen.emit(IR_ADD, tmp, idName, "1");
                irgen.emit(IR_ASSIGN, idName, tmp);
            } else if(current.type==T_DEC){
                advance();
                semantic.noteUse(idName,idLine,"decrement");
                string tmp = irgen.newTemp();
                irgen.emit(IR_SUB, tmp, idName, "1");
                irgen.emit(IR_ASSIGN, idName, tmp);
            } else if(current.type==T_ASSIGN){
                advance();
                ExprResult rhs = expression();
                irgen.emit(IR_ASSIGN, idName, rhs.place);
            } else {
                expression();
            }
        } else {
            expression();
        }
    }
    if(current.type==T_RPAREN) advance();
    else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";

    // Extract increment instructions and remove from stream temporarily
    int incEnd = (int)irgen.getInstructions().size();
    vector<IRInstruction> incCode(
        irgen.getInstructions().begin()+incStart,
        irgen.getInstructions().end());
    irgen.getInstructions().erase(
        irgen.getInstructions().begin()+incStart,
        irgen.getInstructions().end());

    // Body
    if(current.type==T_LBRACE) block(); else statement();

    // Re-append increment code after body
    for(auto& i : incCode) irgen.getInstructions().push_back(i);

    irgen.emit(IR_GOTO,  labelStart);
    irgen.emit(IR_LABEL, labelEnd);
    semantic.logForLoop(current.line);
}

// ─────────────────────────────────────────────────────────────────────────────
// Return statement
// ─────────────────────────────────────────────────────────────────────────────
void Parser::returnStatement() {
    int retLine = current.line;
    advance(); // 'return'
    string retPlace, retType;
    if(current.type!=T_SEMICOLON){
        ExprResult er = expression();
        retPlace = er.place;
        retType  = er.type;
    }
    if(current.type==T_SEMICOLON) advance();
    else cout<<"Syntax Error: expected ';' at "<<fileName<<":"<<current.line<<"\n";

    semantic.noteReturn(retLine, retType);
    semantic.logReturn(retLine);
    irgen.emit(IR_RETURN, "", retPlace);
}

// ─────────────────────────────────────────────────────────────────────────────
// Expression parsing — returns ExprResult{type, place}
// ─────────────────────────────────────────────────────────────────────────────
ExprResult Parser::expression()    { return parseLogical(); }

ExprResult Parser::parseLogical() {
    ExprResult left = parseRelational();
    while(current.type==T_AND || current.type==T_OR){
        IROpCode op = (current.type==T_AND) ? IR_AND : IR_OR;
        advance();
        ExprResult right = parseRelational();
        if(left.type!="bool"||right.type!="bool")
            semantic.logError("Logical operator requires bool operands",current.line);
        string tmp = irgen.newTemp();
        irgen.emit(op, tmp, left.place, right.place);
        left = {"bool", tmp};
    }
    return left;
}

ExprResult Parser::parseRelational() {
    ExprResult left = parseTerm();
    while(current.type==T_LT||current.type==T_LE||current.type==T_GT||
          current.type==T_GE||current.type==T_EQ||current.type==T_NE){
        TokenType tok = current.type;
        IROpCode op;
        if(tok==T_LT) op=IR_LT; else if(tok==T_LE) op=IR_LE;
        else if(tok==T_GT) op=IR_GT; else if(tok==T_GE) op=IR_GE;
        else if(tok==T_EQ) op=IR_EQ; else op=IR_NE;
        advance();
        ExprResult right = parseTerm();
        if(left.type!=right.type)
            semantic.logError("Type mismatch in relational expression",current.line);
        string tmp = irgen.newTemp();
        irgen.emit(op, tmp, left.place, right.place);
        left = {"bool", tmp};
    }
    return left;
}

ExprResult Parser::parseTerm() {
    ExprResult left = parseFactor();
    while(current.type==T_PLUS||current.type==T_MINUS){
        IROpCode op = (current.type==T_PLUS) ? IR_ADD : IR_SUB;
        advance();
        ExprResult right = parseFactor();
        if(!(left.type=="int"&&right.type=="int"))
            semantic.logError("Arithmetic operator requires int operands",current.line);
        string tmp = irgen.newTemp();
        irgen.emit(op, tmp, left.place, right.place);
        left = {"int", tmp};
    }
    return left;
}

ExprResult Parser::parseFactor() {
    ExprResult left = parsePrimary();
    while(current.type==T_MUL||current.type==T_DIV){
        IROpCode op = (current.type==T_MUL) ? IR_MUL : IR_DIV;
        advance();
        ExprResult right = parsePrimary();
        if(!(left.type=="int"&&right.type=="int"))
            semantic.logError("Arithmetic operator requires int operands",current.line);
        string tmp = irgen.newTemp();
        irgen.emit(op, tmp, left.place, right.place);
        left = {"int", tmp};
    }
    return left;
}

ExprResult Parser::parsePrimary() {
    // Unary NOT
    if(current.type==T_NOT){
        advance();
        ExprResult inner = parsePrimary();
        if(inner.type!="bool")
            semantic.logError("'!' requires bool operand",current.line);
        string tmp = irgen.newTemp();
        irgen.emit(IR_NOT, tmp, inner.place);
        return {"bool", tmp};
    }

    // Integer literal
    if(current.type==T_NUMBER){
        string val = current.value; advance();
        return {"int", val};
    }
    // Char literal
    if(current.type==T_CHAR_LITERAL){
        string val = "'"+current.value+"'"; advance();
        return {"char", val};
    }
    // Bool literals
    if(current.type==T_TRUE)  { advance(); return {"bool","true"}; }
    if(current.type==T_FALSE) { advance(); return {"bool","false"}; }

    // Identifier — variable, array access, or function call
    if(current.type==T_IDENTIFIER){
        string name = current.value;
        int    line = current.line;
        advance();

        if(current.type==T_LBRACKET){
            // Array element read: arr[idx]
            advance();
            string idxPlace; string idxType;
            if(current.type==T_NUMBER){
                int idx=stoi(current.value);
                semantic.checkArrayIndex(name,idx,line);
                idxPlace=to_string(idx); idxType="int"; advance();
            } else {
                ExprResult er=expression();
                idxPlace=er.place; idxType=er.type;
                semantic.checkArrayIndexType(idxType,line);
            }
            if(current.type==T_RBRACKET) advance();
            else cout<<"Syntax Error: expected ']' at "<<fileName<<":"<<current.line<<"\n";
            // Determine element type
            string elemType = semantic.getVariableType(name,line);
            string tmp = irgen.newTemp();
            irgen.emit(IR_ARRAY_LOAD, tmp, name, idxPlace);
            return {elemType, tmp};
        }

        if(current.type==T_LPAREN){
            // Function call expression: f(args)
            advance();
            vector<string> argTypes, argPlaces;
            if(current.type!=T_RPAREN){
                ExprResult a=expression();
                argTypes.push_back(a.type); argPlaces.push_back(a.place);
                while(current.type==T_COMMA){
                    advance();
                    ExprResult b=expression();
                    argTypes.push_back(b.type); argPlaces.push_back(b.place);
                }
            }
            if(current.type==T_RPAREN) advance();
            else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";

            semantic.logFunctionCall(name,line);
            semantic.checkFunctionCall(name,argTypes,line);
            string retType = semantic.getFunctionReturnType(name,line);
            for(auto& p:argPlaces) irgen.emit(IR_PARAM,"",p);
            string tmp = (retType.empty()||retType=="void") ? "" : irgen.newTemp();
            irgen.emit(IR_CALL, tmp, name, to_string(argPlaces.size()));
            return {retType, tmp};
        }

        // Plain variable reference
        string varType = semantic.getVariableType(name,line);
        semantic.noteUse(name,line,"expression");
        return {varType, name};
    }

    // Parenthesised expression
    if(current.type==T_LPAREN){
        advance();
        ExprResult inner = expression();
        if(current.type==T_RPAREN) advance();
        else cout<<"Syntax Error: expected ')' at "<<fileName<<":"<<current.line<<"\n";
        return inner;
    }

    cout<<"Syntax Error: expected number, identifier, character literal, "
        <<"'true', 'false', or '(' at "<<fileName<<":"<<current.line<<"\n";
    return {"", ""};
}

// Stub implementations kept for linker compatibility
void Parser::parameterList(){}
void Parser::argumentList() {}
TokenType Parser::parseType(){ return T_INT; }
