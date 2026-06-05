#pragma once
#include "ir.h"
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
using namespace std;

// ── Runtime value ────────────────────────────────────────────────────────────
struct Value {
    enum Kind { INT, BOOL, CHAR_VAL, UNDEF } kind = UNDEF;
    int  intVal  = 0;
    bool boolVal = false;
    char charVal = 0;

    static Value ofInt (int  v){ Value r; r.kind=INT;      r.intVal =v; return r; }
    static Value ofBool(bool v){ Value r; r.kind=BOOL;     r.boolVal=v; return r; }
    static Value ofChar(char v){ Value r; r.kind=CHAR_VAL; r.charVal=v; return r; }
    static Value undef ()      { return Value(); }

    string str() const {
        switch(kind){
            case INT:      return to_string(intVal);
            case BOOL:     return boolVal ? "true" : "false";
            case CHAR_VAL: return string(1,charVal);
            default:       return "<undef>";
        }
    }
    int asInt() const {
        if (kind==INT)  return intVal;
        if (kind==BOOL) return boolVal ? 1 : 0;
        if (kind==CHAR_VAL) return (int)charVal;
        return 0;
    }
    bool asBool() const {
        if (kind==BOOL) return boolVal;
        if (kind==INT)  return intVal != 0;
        return false;
    }
};

// ── Interpreter / virtual machine ────────────────────────────────────────────
class Interpreter {
private:
    const vector<IRInstruction>& ir;
    bool debugMode;

    // Function table: funcName -> index of its IR_FUNC_BEGIN instruction
    map<string,int> funcStart;
    // Label table: labelName -> instruction index
    map<string,int> labelIdx;

    // Call stack frames
    struct Frame {
        string funcName;
        map<string,Value>         vars;      // scalars and temporaries
        map<string,vector<Value>> arrays;    // arrays
        int returnIp = -1;                   // where to resume after RETURN
        Value returnValue;                   // value returned by this call
    };
    vector<Frame> callStack;
    vector<Value> argBuf;    // staging area for PARAM before CALL

    Frame& top() { return callStack.back(); }

    // Resolve a name to a Value (literal or variable lookup)
    Value resolve(const string& name);
    // Store a Value into the current frame
    void  store (const string& name, Value v);

    // Execute one instruction; returns new ip (-1 = done/return)
    int executeOne(int ip);

    // Build function and label index tables
    void buildIndex();

public:
    Interpreter(const vector<IRInstruction>& ir, bool debug = false)
        : ir(ir), debugMode(debug) {}

    // Run the program starting at main(); returns main's return value
    Value run();
};
