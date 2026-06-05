#include "ir.h"
using namespace std;

string IRGenerator::opSymbol(IROpCode op) {
    switch (op) {
        case IR_ADD: return "+";   case IR_SUB: return "-";
        case IR_MUL: return "*";   case IR_DIV: return "/";
        case IR_LT:  return "<";   case IR_LE:  return "<=";
        case IR_GT:  return ">";   case IR_GE:  return ">=";
        case IR_EQ:  return "==";  case IR_NE:  return "!=";
        case IR_AND: return "&&";  case IR_OR:  return "||";
        default:     return "?";
    }
}

string IRInstruction::toString() const {
    if (isDead) return "";
    switch (op) {
        case IR_FUNC_BEGIN: return "\nFUNCTION " + result + " -> " + arg1 + " {";
        case IR_FUNC_END:   return "} // end " + result;
        case IR_DECLARE:    return "    DECLARE " + result + " : " + arg1;
        case IR_LABEL:      return "  " + result + ":";
        case IR_GOTO:       return "    GOTO " + result;
        case IR_IF_FALSE:   return "    IF_FALSE " + arg1 + " GOTO " + result;
        case IR_ASSIGN:     return "    " + result + " = " + arg1;
        case IR_NOT:        return "    " + result + " = !" + arg1;
        case IR_NEG:        return "    " + result + " = -" + arg1;
        case IR_PARAM:      return "    PARAM " + arg1;
        case IR_CALL:
            if (result.empty()) return "    CALL " + arg1 + ", " + arg2;
            else                return "    " + result + " = CALL " + arg1 + ", " + arg2;
        case IR_RETURN:
            if (arg1.empty()) return "    RETURN";
            else              return "    RETURN " + arg1;
        case IR_ARRAY_STORE: return "    " + result + "[" + arg2 + "] = " + arg1;
        case IR_ARRAY_LOAD:  return "    " + result + " = " + arg1 + "[" + arg2 + "]";
        default:
            return "    " + result + " = " + arg1 + " " + IRGenerator::opSymbol(op) + " " + arg2;
    }
}

void IRGenerator::print() const {
    cout << "\n========== Three-Address Code (TAC) ==========\n";
    for (const auto& I : instructions) {
        if (I.isDead) continue;
        switch (I.op) {
            case IR_FUNC_BEGIN:
                cout << "\nFUNCTION " << I.result << " -> " << I.arg1 << " {\n";
                break;
            case IR_FUNC_END:
                cout << "} // end " << I.result << "\n";
                break;
            case IR_DECLARE:
                cout << "    DECLARE " << I.result << " : " << I.arg1 << "\n";
                break;
            case IR_LABEL:
                cout << "  " << I.result << ":\n";
                break;
            case IR_GOTO:
                cout << "    GOTO " << I.result << "\n";
                break;
            case IR_IF_FALSE:
                cout << "    IF_FALSE " << I.arg1 << " GOTO " << I.result << "\n";
                break;
            case IR_ASSIGN:
                cout << "    " << I.result << " = " << I.arg1 << "\n";
                break;
            case IR_NOT:
                cout << "    " << I.result << " = !" << I.arg1 << "\n";
                break;
            case IR_NEG:
                cout << "    " << I.result << " = -" << I.arg1 << "\n";
                break;
            case IR_PARAM:
                cout << "    PARAM " << I.arg1 << "\n";
                break;
            case IR_CALL:
                if (I.result.empty())
                    cout << "    CALL " << I.arg1 << ", " << I.arg2 << "\n";
                else
                    cout << "    " << I.result << " = CALL " << I.arg1 << ", " << I.arg2 << "\n";
                break;
            case IR_RETURN:
                if (I.arg1.empty()) cout << "    RETURN\n";
                else                cout << "    RETURN " << I.arg1 << "\n";
                break;
            case IR_ARRAY_STORE:
                cout << "    " << I.result << "[" << I.arg2 << "] = " << I.arg1 << "\n";
                break;
            case IR_ARRAY_LOAD:
                cout << "    " << I.result << " = " << I.arg1 << "[" << I.arg2 << "]\n";
                break;
            // Binary ops
            default:
                cout << "    " << I.result << " = " << I.arg1
                     << " " << opSymbol(I.op) << " " << I.arg2 << "\n";
                break;
        }
    }
    cout << "==============================================\n";
}
