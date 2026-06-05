#pragma once 
#include <string>
#include <map> // We use maps (key-value dictionaries)
#include <vector> // We use lists (vectors)
#include <iostream> 

using namespace std; 

struct SymbolInfo { // This holds information about one variable or parameter
    string name; // The name of the variable (like "count")
    string type;      // "int", "char", "bool", "void" // The data type of the variable
    bool isArray = false; // True if this name represents an array
    int arraySize = -1; // The size of the array (-1 means unknown or not an array)
    int line = 0; // The line number where it was declared (for messages)
    string kind;      // "variable", "parameter", "function" // What kind of symbol it is
    bool initialized = false; // True after we see it assigned a value
    int scopeDepth = 0; // Which scope level it belongs to (0 is global)
};

struct FunctionInfo { // This holds information about one function
    string name; // Function name (like "main")
    string returnType; // What type the function returns (like "int", "void")
    vector<string> paramTypes; // Types of the function parameters (like ["int","char"])
    vector<string> paramNames; // Names of the function parameters (like ["n","c"])
    int line = 0; // Line number of declaration/definition
    bool hasDefinition = false; // True if we saw the function body { ... }
    bool hasReturnStatement = false; // True if we saw a return inside the body (for non-void)
};

class Semantic { // This class does semantic checks and keeps track of symbols
public:
    Semantic(const string& fileName); // Create the semantic helper and remember the file name

    // Scope management
    void enterScope(); // Start a new scope (like entering a block { })
    void leaveScope(); // End the current scope (leave the block)

    // Declarations
    bool declareVariable(const string& name, const string& type, bool isArray, int arraySize, int line, const string& kind); // Add a new variable to the current scope and check for errors
    void noteInitialization(const string& name, int line); // Mark the variable as initialized (assigned a value)
    void noteUse(const string& name, int line, const string& context); // Record that a variable was used and check for errors

    // Functions
    bool declareFunction(const string& name, // Record a function’s signature and check for conflicts
                         const string& returnType, // The function’s return type
                         const vector<string>& paramTypes, // The types of its parameters
                         const vector<string>& paramNames, // The names of its parameters
                         int line, // Where in the file this appears
                         bool isDefinition); // True if this includes the body, false if just a declaration

    void enterFunctionBody(const string& name); // Start tracking inside a function body (open function scope)
    void leaveFunctionBody(); // Stop tracking inside the function body (close function scope)
    void noteReturn(int line, const string& returnedType); // Record a return statement and check its type

    // Logging
    void logInfo(const string& message, int line); // Save an informational message
    void logWarning(const string& message, int line); // Save a warning message
    void logError(const string& message, int line); // Save an error message

    // Specific helpers
    void logDeclarationParsed(const string& name, const string& typeRepr, int line, const string& initializer = ""); // Log that a declaration was parsed
    void logAssignment(const string& name, const string& exprRepr, int line); // Log that an assignment happened
    void logIfStatement(int line); // Log that we parsed an if-else
    void logForLoop(int line); // Log that we parsed a for loop
    void logReturn(int line); // Log that we parsed a return
    void logFunctionCall(const string& name, int line); // Log that a function was called

    // Expression type checking helpers
    string getVariableType(const string& name, int line); // Ask for a variable’s type by name (with error if missing)
    void checkArrayIndex(const string& name, int index, int line); // Check a constant array index is inside bounds
    void checkArrayIndexType(const std::string& indexType, int line); // ensure array index is int type
    void checkAssignmentType(const std::string& varName, const std::string& rhsType, int line); // check RHS type matches variable type
    void checkArrayElementAssignment(const std::string& varName, 
                                     const std::string& elementType, 
                                     int line); // check RHS type matches array element type
    // Final reporting / checks
    void finalize(); // At the end, print messages and run final checks (like main’s rules)
    bool checkFunctionCall(const string& name,
                           const vector<std::string>& argTypes,
                           int line); // verify function exists and signature matches
    string getFunctionReturnType(const std::string& name, int line); // fetch declared return type

private:
    string fileName; // The source file name (used for messages)
    vector<map<string, SymbolInfo>> scopes; // A stack of scopes; each scope is a map of name -> SymbolInfo
    map<string, FunctionInfo> functions; // A map of function name -> FunctionInfo
    string currentFunction; // The name of the function we are currently inside (empty if none)
    bool hasSemanticError = false; // True if we have seen any semantic error

    // main tracking
    bool mainSeen = false; // True after we see a 'main' function
    int mainFirstLine = 0; // The first line where main appeared
    int mainRedeclLine = 0; // Line of a redeclaration/redefinition of main (if any)
    bool mainSignatureValid = true; // True if main has the correct signature (int main())
    string mainReturnType; // The return type of main that we recorded

    vector<string> infoLogs; // Stored info messages to print later
    vector<string> warningLogs; // Stored warning messages to print later
    vector<string> errorLogs; // Stored error messages to print later

    SymbolInfo* lookup(const string& name); // Find a symbol by name, searching inner to outer scopes
    SymbolInfo* lookupInCurrentScope(const string& name); // Find a symbol only in the current scope

    void checkUninitializedAtUse(SymbolInfo* sym, int line, const string& context); // Warn if a variable is used before being initialized
    void checkMainConstraints(); // Check special rules for the main function and log issues
};
