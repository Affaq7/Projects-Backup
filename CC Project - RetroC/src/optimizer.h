#pragma once
#include "ir.h"
#include <string>
#include <cctype>
using namespace std;

// Applies optimisation passes over the TAC instruction list.
class Optimizer {
private:
    vector<IRInstruction>& instructions;

    // Returns true if s is a decimal integer literal (possibly negative)
    bool isIntLit(const string& s) const {
        if (s.empty()) return false;
        size_t start = (s[0] == '-') ? 1 : 0;
        if (start >= s.size()) return false;
        for (size_t i = start; i < s.size(); i++)
            if (!isdigit((unsigned char)s[i])) return false;
        return true;
    }

    int toInt(const string& s) const { return stoi(s); }

public:
    explicit Optimizer(vector<IRInstruction>& instr) : instructions(instr) {}

    // Pass 1 — Constant Folding
    // If both operands of an arithmetic / relational op are integer literals,
    // compute the result at compile time and replace with a plain ASSIGN.
    int constantFolding();

    // Pass 2 — Dead Code Elimination
    // Any instruction that follows an unconditional RETURN or GOTO inside the
    // same function body (before the next label) is unreachable and removed.
    int deadCodeElimination();

    // Run all passes and print a summary
    void optimize();
};
