#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "parser.h"
#include "optimizer.h"
#include "interpreter.h"
using namespace std;

// ─── usage ────────────────────────────────────────────────────────────────────
static void printHelp(const char* prog) {
    cout << "\nUsage:\n"
         << "  " << prog << " <input.rc>               -- compile and run\n"
         << "  " << prog << " <input.rc> -o <out.txt>  -- write generated assembly/output to file\n"
         << "  " << prog << " <input.rc> --debug       -- show TAC + optimizations + VM trace\n"
         << "  " << prog << " <input.rc> --ir          -- show Three-Address Code only\n"
         << "  " << prog << " --interactive            -- REPL mode (type expressions interactively)\n"
         << "  " << prog << " --help                   -- show this message\n\n"
         << "Supported source file extensions: .rc, .cc, .txt\n\n";
}

// ─── Simple REPL ─────────────────────────────────────────────────────────────
static void runRepl() {
    cout << "\n======================================\n";
    cout << "  RetroC Compiler  --  REPL Mode\n";
    cout << "  Type a complete program and end with\n";
    cout << "  a line containing only 'END'\n";
    cout << "======================================\n";

    while (true) {
        cout << "\nretroC> ";
        string src, line;
        while (getline(cin, line)) {
            if (line == "END" || line == "end" || line == "quit" || line == "exit") break;
            src += line + "\n";
        }
        if (src.empty() || line == "quit" || line == "exit") {
            cout << "Goodbye!\n";
            break;
        }

        cout << "\n--- Compiling ---\n";
        Scanner   sc(src);
        Parser    parser(sc, "<repl>");
        parser.parse();

        Optimizer opt(parser.getIR().getInstructions());
        opt.optimize();

        parser.getIR().print();

        cout << "\n--- Executing ---\n";
        try {
            Interpreter vm(parser.getIR().getInstructions(), false);
            Value ret = vm.run();
            cout << "\nProgram exited with return value: " << ret.str() << "\n";
        } catch (const exception& e) {
            cout << "Runtime Error: " << e.what() << "\n";
        }
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Flags ────────────────────────────────────────────────────────────────
    bool debugMode   = false;
    bool showIR      = false;
    bool interactive = false;
    string inputFile;
    string outputFile;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug")       debugMode   = true;
        else if (arg == "--ir")     showIR      = true;
        else if (arg == "--interactive") interactive = true;
        else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "--help")   { printHelp(argv[0]); return 0; }
        else if (arg[0] != '-')     inputFile   = arg;
    }

    // ── REPL mode ─────────────────────────────────────────────────────────────
    if (interactive) { runRepl(); return 0; }

    // ── File mode ─────────────────────────────────────────────────────────────
    if (inputFile.empty()) inputFile = "input.rc"; // default for backwards compat

    ifstream file(inputFile);
    if (!file.is_open()) {
        cout << "Error: Could not open '" << inputFile << "'\n";
        printHelp(argv[0]);
        return 1;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    cout << "\n========== Compiling: " << inputFile << " ==========\n";

    // ── Phase 1-2-3: Scan + Parse + Semantic ─────────────────────────────────
    Scanner   sc(buffer.str());
    Parser    parser(sc, inputFile);
    parser.parse();  // prints Syntax/Semantic errors automatically

    // ── Phase 4: IR ───────────────────────────────────────────────────────────
    IRGenerator& irgen = parser.getIR();
    if (showIR || debugMode) irgen.print();

    // ── Phase 5: Optimisation ─────────────────────────────────────────────────
    Optimizer opt(irgen.getInstructions());
    opt.optimize();

    // Print optimised IR in debug mode
    if (debugMode) {
        cout << "\n--- Optimised TAC ---\n";
        irgen.print();
    }

    // ── Phase 6: Interpret / Execute ─────────────────────────────────────────
    cout << "\n========== Executing ==========\n";
    try {
        Interpreter vm(irgen.getInstructions(), debugMode);
        Value ret = vm.run();
        cout << "\nProgram finished. Return value of main(): " << ret.str() << "\n";

        if (!outputFile.empty()) {
            ofstream out(outputFile);
            if (out.is_open()) {
                out << "=== Generated Custom Assembly (TAC) ===\n";
                for (const auto& inst : irgen.getInstructions()) {
                    if (inst.isDead) continue;
                    out << inst.toString() << "\n";
                }
                out << "\n=== Execution Result ===\n";
                out << "Return value: " << ret.str() << "\n";
                out.close();
                cout << "\n[Success] Output written to " << outputFile << "\n";
            } else {
                cout << "\n[Error] Could not open output file: " << outputFile << "\n";
            }
        }
    } catch (const exception& e) {
        cout << "Runtime Error: " << e.what() << "\n";
    }

    return 0;
}
