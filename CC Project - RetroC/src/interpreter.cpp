#include "interpreter.h"
#include <iostream>
#include <stdexcept>
#include <cctype>
using namespace std;

// ── Build look-up tables ─────────────────────────────────────────────────────
void Interpreter::buildIndex() {
    for (int i = 0; i < (int)ir.size(); i++) {
        if (ir[i].isDead) continue;
        if (ir[i].op == IR_FUNC_BEGIN) funcStart[ir[i].result] = i;
        if (ir[i].op == IR_LABEL)      labelIdx [ir[i].result] = i;
    }
}

// ── Resolve a name to a runtime Value ────────────────────────────────────────
Value Interpreter::resolve(const string& name) {
    if (name.empty()) return Value::undef();

    // Boolean literals
    if (name == "true")  return Value::ofBool(true);
    if (name == "false") return Value::ofBool(false);

    // Character literal  'x'
    if (name.size() == 1 && !isdigit((unsigned char)name[0]) && !isalpha((unsigned char)name[0]))
        return Value::ofChar(name[0]);
    if (name.size() == 3 && name[0]=='\'' && name[2]=='\'')
        return Value::ofChar(name[1]);

    // Integer literal (possibly negative)
    bool isNum = true;
    size_t s = (name[0]=='-') ? 1 : 0;
    for (size_t i=s; i<name.size(); i++) if(!isdigit((unsigned char)name[i])){ isNum=false; break; }
    if (isNum && s < name.size()) return Value::ofInt(stoi(name));

    // Variable look-up (search all frames from top)
    for (int f = (int)callStack.size()-1; f >= 0; f--) {
        auto it = callStack[f].vars.find(name);
        if (it != callStack[f].vars.end()) return it->second;
    }
    return Value::undef();
}

void Interpreter::store(const string& name, Value v) {
    top().vars[name] = v;
}

// ── Execute one instruction, return next ip ───────────────────────────────────
int Interpreter::executeOne(int ip) {
    const IRInstruction& I = ir[ip];
    if (I.isDead) return ip + 1;

    if (debugMode) {
        cout << "  [VM ip=" << ip << "] ";
    }

    switch (I.op) {
        // ── bookkeeping ─────────────────────────────────────────────────────
        case IR_FUNC_BEGIN:
        case IR_FUNC_END:
        case IR_DECLARE:
        case IR_LABEL:
            if (debugMode) cout << "(skip structural)\n";
            return ip + 1;

        // ── ASSIGN ──────────────────────────────────────────────────────────
        case IR_ASSIGN: {
            Value v = resolve(I.arg1);
            store(I.result, v);
            if (debugMode) cout << I.result << " = " << v.str() << "\n";
            return ip + 1;
        }

        // ── Binary arithmetic ────────────────────────────────────────────────
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: {
            int a = resolve(I.arg1).asInt(), b = resolve(I.arg2).asInt(), r = 0;
            if      (I.op==IR_ADD) r = a + b;
            else if (I.op==IR_SUB) r = a - b;
            else if (I.op==IR_MUL) r = a * b;
            else if (I.op==IR_DIV) { if(b==0) throw runtime_error("Division by zero"); r = a/b; }
            store(I.result, Value::ofInt(r));
            if (debugMode) cout << I.result << " = " << a << " op " << b << " = " << r << "\n";
            return ip + 1;
        }

        // ── Relational ───────────────────────────────────────────────────────
        case IR_LT: case IR_LE: case IR_GT:
        case IR_GE: case IR_EQ: case IR_NE: {
            int a = resolve(I.arg1).asInt(), b = resolve(I.arg2).asInt();
            bool r = false;
            if      (I.op==IR_LT) r = a <  b;
            else if (I.op==IR_LE) r = a <= b;
            else if (I.op==IR_GT) r = a >  b;
            else if (I.op==IR_GE) r = a >= b;
            else if (I.op==IR_EQ) r = a == b;
            else if (I.op==IR_NE) r = a != b;
            store(I.result, Value::ofBool(r));
            if (debugMode) cout << I.result << " = " << (r?"true":"false") << "\n";
            return ip + 1;
        }

        // ── Logical ──────────────────────────────────────────────────────────
        case IR_AND: {
            bool r = resolve(I.arg1).asBool() && resolve(I.arg2).asBool();
            store(I.result, Value::ofBool(r));
            if (debugMode) cout << I.result << " = " << (r?"true":"false") << "\n";
            return ip + 1;
        }
        case IR_OR: {
            bool r = resolve(I.arg1).asBool() || resolve(I.arg2).asBool();
            store(I.result, Value::ofBool(r));
            if (debugMode) cout << I.result << " = " << (r?"true":"false") << "\n";
            return ip + 1;
        }
        case IR_NOT: {
            bool r = !resolve(I.arg1).asBool();
            store(I.result, Value::ofBool(r));
            if (debugMode) cout << I.result << " = " << (r?"true":"false") << "\n";
            return ip + 1;
        }
        case IR_NEG: {
            int r = -resolve(I.arg1).asInt();
            store(I.result, Value::ofInt(r));
            if (debugMode) cout << I.result << " = " << r << "\n";
            return ip + 1;
        }

        // ── Control flow ─────────────────────────────────────────────────────
        case IR_GOTO: {
            if (debugMode) cout << "GOTO " << I.result << "\n";
            auto it = labelIdx.find(I.result);
            if (it == labelIdx.end()) throw runtime_error("Unknown label: " + I.result);
            return it->second + 1;
        }
        case IR_IF_FALSE: {
            bool cond = resolve(I.arg1).asBool();
            if (debugMode) cout << "IF_FALSE " << I.arg1 << "=" << (cond?"true":"false") << "\n";
            if (!cond) {
                auto it = labelIdx.find(I.result);
                if (it == labelIdx.end()) throw runtime_error("Unknown label: " + I.result);
                return it->second + 1;
            }
            return ip + 1;
        }

        // ── Function call ────────────────────────────────────────────────────
        case IR_PARAM:
            argBuf.push_back(resolve(I.arg1));
            if (debugMode) cout << "PARAM " << resolve(I.arg1).str() << "\n";
            return ip + 1;

        case IR_CALL: {
            string fname = I.arg1;
            if (debugMode) cout << "CALL " << fname << "\n";
            auto it = funcStart.find(fname);
            if (it == funcStart.end()) throw runtime_error("Undefined function: " + fname);

            // Set up new frame
            Frame frame;
            frame.funcName  = fname;
            frame.returnIp  = ip + 1;

            // Bind arguments to parameter names
            int funcIp = it->second; // IR_FUNC_BEGIN
            int argIdx = 0;
            for (int k = funcIp + 1; k < (int)ir.size() && argIdx < (int)argBuf.size(); k++) {
                if (ir[k].op == IR_DECLARE && !ir[k].result.empty()) {
                    // parameters are declared first in function body
                    frame.vars[ir[k].result] = argBuf[argIdx++];
                    if (argIdx == (int)argBuf.size()) break;
                }
                if (ir[k].op == IR_FUNC_END) break;
            }
            argBuf.clear();
            callStack.push_back(frame);
            return funcIp + 1; // start executing body
        }

        // ── Return ───────────────────────────────────────────────────────────
        case IR_RETURN: {
            Value retVal = I.arg1.empty() ? Value::undef() : resolve(I.arg1);
            if (debugMode) cout << "RETURN " << retVal.str() << "\n";
            int resumeIp = top().returnIp;
            // Store return value so caller can grab it
            string callerResult;
            // Find the CALL instruction at resumeIp-1 to get result name
            if (resumeIp > 0 && ir[resumeIp-1].op == IR_CALL)
                callerResult = ir[resumeIp-1].result;
            callStack.pop_back();
            if (!callStack.empty() && !callerResult.empty())
                store(callerResult, retVal);
            if (callStack.empty()) {
                // Returned from main — stash result for run()
                callStack.push_back(Frame{});   // sentinel
                top().returnValue = retVal;
                return -1; // signal done
            }
            return resumeIp;
        }

        // ── Arrays ───────────────────────────────────────────────────────────
        case IR_ARRAY_STORE: {
            int idx = resolve(I.arg2).asInt();
            auto& arr = top().arrays[I.result];
            if ((int)arr.size() <= idx) arr.resize(idx+1, Value::undef());
            arr[idx] = resolve(I.arg1);
            if (debugMode) cout << I.result << "[" << idx << "] = " << resolve(I.arg1).str() << "\n";
            return ip + 1;
        }
        case IR_ARRAY_LOAD: {
            int idx = resolve(I.arg2).asInt();
            Value v = Value::undef();
            auto& arr = top().arrays[I.arg1];
            if (idx >= 0 && idx < (int)arr.size()) v = arr[idx];
            store(I.result, v);
            if (debugMode) cout << I.result << " = " << I.arg1 << "[" << idx << "] = " << v.str() << "\n";
            return ip + 1;
        }

        default:
            return ip + 1;
    }
}

// ── Run from main() ──────────────────────────────────────────────────────────
Value Interpreter::run() {
    buildIndex();
    auto it = funcStart.find("main");
    if (it == funcStart.end()) throw runtime_error("No 'main' function found");

    Frame mainFrame;
    mainFrame.funcName = "main";
    mainFrame.returnIp = -1;
    callStack.push_back(mainFrame);

    int ip = it->second + 1; // skip IR_FUNC_BEGIN
    int limit = 100000;       // guard against infinite loops
    while (ip >= 0 && ip < (int)ir.size() && --limit > 0) {
        ip = executeOne(ip);
    }
    if (limit == 0) cout << "  [VM] Warning: execution limit reached (possible infinite loop)\n";

    // Retrieve stored return value from sentinel frame
    Value ret = callStack.empty() ? Value::undef() : callStack.back().returnValue;
    callStack.clear();
    return ret;
}
