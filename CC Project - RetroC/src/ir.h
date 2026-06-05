#pragma once
#include <string>
#include <vector>
#include <iostream>
using namespace std;

// Opcodes for Three-Address Code (TAC) instructions
enum IROpCode {
    IR_FUNC_BEGIN,   // begin of a function: result=funcName, arg1=retType
    IR_FUNC_END,     // end of a function:   result=funcName
    IR_DECLARE,      // declare variable:    result=varName, arg1=type
    IR_ASSIGN,       // result = arg1
    IR_ADD,          // result = arg1 + arg2
    IR_SUB,          // result = arg1 - arg2
    IR_MUL,          // result = arg1 * arg2
    IR_DIV,          // result = arg1 / arg2
    IR_LT,           // result = arg1 < arg2  (bool)
    IR_LE,           // result = arg1 <= arg2
    IR_GT,           // result = arg1 > arg2
    IR_GE,           // result = arg1 >= arg2
    IR_EQ,           // result = arg1 == arg2
    IR_NE,           // result = arg1 != arg2
    IR_AND,          // result = arg1 && arg2
    IR_OR,           // result = arg1 || arg2
    IR_NOT,          // result = !arg1
    IR_NEG,          // result = -arg1
    IR_LABEL,        // label declaration:   result=labelName
    IR_GOTO,         // unconditional jump:  result=labelName
    IR_IF_FALSE,     // if !arg1 goto result
    IR_PARAM,        // push function argument: arg1=value
    IR_CALL,         // result = call arg1, arg2=argCount (result may be "" for void)
    IR_RETURN,       // return arg1 (arg1="" for void return)
    IR_ARRAY_STORE,  // result[arg2] = arg1
    IR_ARRAY_LOAD,   // result = arg1[arg2]
};

// A single TAC instruction
struct IRInstruction {
    IROpCode op;
    string   result;   // destination / label name / function name
    string   arg1;     // first operand
    string   arg2;     // second operand
    bool     isDead = false; // marked by dead-code elimination

    IRInstruction(IROpCode op, string result = "", string arg1 = "", string arg2 = "")
        : op(op), result(result), arg1(arg1), arg2(arg2) {}

    string toString() const;
};

// Generates and stores TAC instructions
class IRGenerator {
private:
    vector<IRInstruction> instructions;
    int tempCount  = 0;
    int labelCount = 0;

public:
    // Create a new unique temporary variable name
    string newTemp()  { return "_t" + to_string(tempCount++);  }
    // Create a new unique label name
    string newLabel() { return "_L" + to_string(labelCount++); }

    // Emit one TAC instruction
    void emit(IROpCode op, string result = "", string arg1 = "", string arg2 = "") {
        instructions.emplace_back(op, result, arg1, arg2);
    }

    vector<IRInstruction>&       getInstructions()       { return instructions; }
    const vector<IRInstruction>& getInstructions() const { return instructions; }

    // Pretty-print the full TAC listing
    void print() const;

    // Helper: opcode → symbol string
    static string opSymbol(IROpCode op);
};
