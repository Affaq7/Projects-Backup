#include "optimizer.h"
#include <iostream>
using namespace std;

// ─── Pass 1: Constant Folding ───────────────────────────────────────────────
int Optimizer::constantFolding() {
    int count = 0;
    for (auto& I : instructions) {
        if (I.isDead) continue;

        // ── Arithmetic: both args are integer literals ──────────────────────
        bool arith = (I.op==IR_ADD||I.op==IR_SUB||I.op==IR_MUL||I.op==IR_DIV);
        if (arith && isIntLit(I.arg1) && isIntLit(I.arg2)) {
            int a = toInt(I.arg1), b = toInt(I.arg2), r = 0;
            if      (I.op == IR_ADD) r = a + b;
            else if (I.op == IR_SUB) r = a - b;
            else if (I.op == IR_MUL) r = a * b;
            else if (I.op == IR_DIV) {
                if (b == 0) continue; // skip div-by-zero
                r = a / b;
            }
            cout << "  [Constant Folding] " << I.arg1 << " "
                 << IRGenerator::opSymbol(I.op) << " "
                 << I.arg2 << "  =>  " << r << "\n";
            I.op   = IR_ASSIGN;
            I.arg1 = to_string(r);
            I.arg2 = "";
            count++;
        }

        // ── Relational: both args are integer literals ───────────────────────
        bool rel = (I.op==IR_LT||I.op==IR_LE||I.op==IR_GT||
                    I.op==IR_GE||I.op==IR_EQ||I.op==IR_NE);
        if (rel && isIntLit(I.arg1) && isIntLit(I.arg2)) {
            int a = toInt(I.arg1), b = toInt(I.arg2);
            bool r = false;
            if      (I.op==IR_LT) r = a <  b;
            else if (I.op==IR_LE) r = a <= b;
            else if (I.op==IR_GT) r = a >  b;
            else if (I.op==IR_GE) r = a >= b;
            else if (I.op==IR_EQ) r = a == b;
            else if (I.op==IR_NE) r = a != b;
            cout << "  [Constant Folding] " << I.arg1 << " "
                 << IRGenerator::opSymbol(I.op) << " "
                 << I.arg2 << "  =>  " << (r?"true":"false") << "\n";
            I.op   = IR_ASSIGN;
            I.arg1 = r ? "true" : "false";
            I.arg2 = "";
            count++;
        }
    }
    return count;
}

// ─── Pass 2: Dead Code Elimination ──────────────────────────────────────────
int Optimizer::deadCodeElimination() {
    int count = 0;
    bool dead = false;
    for (auto& I : instructions) {
        // Function boundaries reset liveness
        if (I.op == IR_FUNC_BEGIN || I.op == IR_FUNC_END) { dead = false; continue; }
        // A label makes subsequent code reachable again
        if (I.op == IR_LABEL) { dead = false; continue; }

        if (dead && !I.isDead) {
            cout << "  [Dead Code Eliminated] Unreachable instruction removed\n";
            I.isDead = true;
            count++;
        }
        // After unconditional return or goto, what follows is dead
        if (!I.isDead && (I.op == IR_RETURN || I.op == IR_GOTO))
            dead = true;
    }
    return count;
}

// ─── Run all passes ──────────────────────────────────────────────────────────
void Optimizer::optimize() {
    cout << "\n========== Optimization Phase ==========\n";

    int folded    = constantFolding();
    int removed   = deadCodeElimination();

    cout << "  Constant Folding     : " << folded  << " optimization(s)\n";
    cout << "  Dead Code Eliminated : " << removed << " instruction(s)\n";
    if (folded == 0 && removed == 0)
        cout << "  (no optimizations applicable to this program)\n";

    cout << "========================================\n";
}
