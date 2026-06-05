// Semantic rules handled in this file (13 total):
// 1) Error: Redeclaration of a variable in the same scope.
// 2) Warning: Shadowing a variable declared in an outer scope.
// 3) Tracking: Variable initialization state (initialized vs not).
// 4) Warning: Use of a potentially uninitialized variable.
// 5) Error: Use of an undeclared identifier.
// 6) Record: Function declarations and definitions (signature and body presence).
// 7) Error: Conflicting function declaration (signature mismatch).
// 8) Error: Multiple definitions of the same function.
// 9) Error: Returning a value from a void function.
// 10) Error: Return type mismatch vs function’s declared return type.
// 11) Warning: Non-void function may reach end without a return (in finalize).
// 12) Constraint: main must have signature int main() (validated).
// 13) Error: Multiple definitions/redeclarations of main.
//14) Error: Indexing non-array variable.
//15) Error: Array index out of bounds for constant index.
// Mechanisms supporting rules:
// - Scoped symbol tables (enterScope/leaveScope, lookup, lookupInCurrentScope).
// - Logging of info/warnings/errors for user feedback.
// - Function context tracking during body parsing (enterFunctionBody/leaveFunctionBody).

#include "semantic.h" // Include the header that declares the Semantic class and related types

Semantic::Semantic(const string &fileName) : fileName(fileName) // Constructor: set the file name
{
    // global scope // We start with one global scope
    scopes.push_back(map<string, SymbolInfo>()); // Create an empty map for symbols and push it as the first scope
}

void Semantic::enterScope() // Start a new inner scope (like inside a block)
{
    scopes.push_back(map<string, SymbolInfo>()); // Add a fresh, empty symbol table for this new scope
}

void Semantic::leaveScope() // Leave the current scope
{
    if (scopes.size() > 1) // Make sure we keep the global scope and only remove if more than one
    {
        scopes.pop_back(); // Remove the most recent scope
    }
}

SymbolInfo *Semantic::lookup(const string &name) // Find a symbol by name, searching from inner to outer scopes
{
    for (int i = (int)scopes.size() - 1; i >= 0; --i) // Check scopes from last (innermost) to first (outermost)
    {
        auto it = scopes[i].find(name); // Try to find the name in this scope
        if (it != scopes[i].end())      // If found
        {
            return &it->second; // Return a pointer to the symbol info
        }
    }
    return nullptr; // If not found in any scope, return null
}

SymbolInfo *Semantic::lookupInCurrentScope(const string &name) // Find a symbol only in the current (innermost) scope
{
    if (scopes.empty())
        return nullptr;        // If there are no scopes, return null
    auto &cur = scopes.back(); // Get the current scope (last in the list)
    auto it = cur.find(name);  // Look for the name in the current scope
    if (it != cur.end())
        return &it->second; // If found, return the symbol info
    return nullptr;         // If not found, return null
}

void Semantic::checkArrayIndex(const string& name, int index, int line)
{
    SymbolInfo* sym = lookup(name); // find the symbol
    if (!sym) {
        logError("Use of undeclared identifier '" + name + "'", line);
        hasSemanticError = true;
        return;
    }
    if (!sym->isArray) {
        logError("Indexing non-array variable '" + name + "'", line);
        hasSemanticError = true;
        return;
    }
    if (sym->arraySize >= 0) {
        if (index < 0 || index >= sym->arraySize) {
            logError("Array index out of bounds for '" + name + "': index=" + to_string(index) +
                     ", size=" + to_string(sym->arraySize), line);
            hasSemanticError = true;
        }
    } else {
        // unknown size (e.g., missing or not constant); cannot validate
        logWarning("Array index check skipped for '" + name + "' due to unknown size", line);
    }
}

void Semantic::checkArrayIndexType(const string& indexType, int line)
{
    if (indexType != "int") // Array index must be int type
    {
        logError("Array index must be of type 'int', not '" + indexType + "'", line);
        hasSemanticError = true;
    }
}

bool Semantic::declareVariable(const string &name, const string &type, bool isArray, int arraySize, int line, const string &kind) // Declare a new variable in the current scope
{
    SymbolInfo *existingInCurrent = lookupInCurrentScope(name); // Check if the name already exists in this scope
    SymbolInfo *existingInOuter = nullptr;                      // Prepare to check outer scopes
    if (!existingInCurrent)                                     // Only look in outer scopes if not in current
    {
        existingInOuter = lookup(name); // Search outer scopes for the same name
    }

    if (existingInCurrent) // If already declared in this scope
    {
        logError("Redeclaration of '" + name + "'", line); // Report an error for redeclaration
        hasSemanticError = true;                           // Mark that a semantic error happened
        return false;                                      // Declaration failed
    }

    if (existingInOuter) // If same name exists in an outer scope
    {
        // Shadowing warning // Warn the user that this name hides an outer one
        logWarning(
            "Shadowing of '" + name + "' declared at " + fileName + ":" + to_string(existingInOuter->line) +
                " by " + kind,
            line);
    }

    SymbolInfo sym;                          // Create a new symbol info record
    sym.name = name;                         // Store the variable name
    sym.type = type;                         // Store the variable type (like "int")
    sym.isArray = isArray;                   // Mark if it is an array
    sym.arraySize = arraySize;               // Store the array size (or -1 if unknown/not an array)
    sym.line = line;                         // Remember the line where it was declared
    sym.kind = kind;                         // Store what kind it is (e.g., "variable" or "parameter")
    sym.initialized = false;                 // Start as not initialized
    sym.scopeDepth = (int)scopes.size() - 1; // Save which scope level it belongs to
    scopes.back()[name] = sym;               // Put this symbol into the current scope’s map
    return true;                             // Declaration succeeded
}

void Semantic::noteInitialization(const string &name, int line) // Mark a variable as initialized
{
    SymbolInfo *sym = lookup(name); // Find the variable in any scope
    if (!sym)
        return;              // If not found, do nothing
    sym->initialized = true; // Mark it as initialized
}

void Semantic::checkUninitializedAtUse(SymbolInfo *sym, int line, const string &context) // Warn if a variable is used before being set
{
    if (!sym)
        return;            // If symbol is missing, do nothing here
    if (!sym->initialized) // If it is not initialized
    {
        logWarning("Use of potentially uninitialized variable '" + sym->name + "' in " + context, line); // Warn about possible uninitialized use
    }
}

void Semantic::noteUse(const string &name, int line, const string &context) // Record that a variable was used somewhere
{
    SymbolInfo *sym = lookup(name); // Try to find the variable
    if (!sym)                       // If it does not exist
    {
        logError("Use of undeclared identifier '" + name + "'", line); // Report an error for using a name not declared
        hasSemanticError = true;                                       // Mark semantic error
        return;                                                        // Stop
    }
    checkUninitializedAtUse(sym, line, context); // If found, check if it was initialized first
}

bool Semantic::declareFunction(const string &name,               // Declare a function (or record its definition)
                               const string &returnType,         // Function return type (e.g., "int")
                               const vector<string> &paramTypes, // Parameter types
                               const vector<string> &paramNames, // Parameter names
                               int line,                         // Line number of declaration/definition
                               bool isDefinition)                // True if this includes the body (definition), false if only declaration
{
    auto it = functions.find(name); // Look for an existing record of this function
    if (it != functions.end())      // If we already have this function recorded
    {
        FunctionInfo &existing = it->second;                                        // Get the existing info
        if (existing.returnType != returnType || existing.paramTypes != paramTypes) // If signature differs
        {
            logError("Conflicting declaration of function '" + name + "'", line); // Report conflicting declaration
            hasSemanticError = true;                                              // Mark semantic error
            return false;                                                         // Fail
        }
        if (isDefinition && existing.hasDefinition) // If we already defined it before
        {
            logError("Multiple definitions of function '" + name +
                         "' (first defined at " + fileName + ":" + to_string(existing.line) +
                         ", redefined at " + fileName + ":" + to_string(line) + ")",
                     line);          // Report duplicate definition
            hasSemanticError = true; // Mark error
            return false;            // Fail
        }
        if (isDefinition) // If this is now the definition
        {
            existing.hasDefinition = true; // Mark that it has a body now
            existing.line = line;          // Update the line to the definition line
        }
    }
    else // If the function is not recorded yet
    {
        FunctionInfo f;                 // Create a new function info record
        f.name = name;                  // Set the function name
        f.returnType = returnType;      // Set the return type
        f.paramTypes = paramTypes;      // Copy parameter types
        f.paramNames = paramNames;      // Copy parameter names
        f.line = line;                  // Remember the line
        f.hasDefinition = isDefinition; // Whether it has a body yet
        f.hasReturnStatement = false;   // Start with no return encountered
        functions[name] = f;            // Save it in the function table
    }

    if (name == "main") // Special checks for main
    {
        if (!mainSeen) // If first time seeing main
        {
            mainSeen = true;             // Note that we saw main
            mainFirstLine = line;        // Save first line of main
            mainReturnType = returnType; // Save return type of main
        }
        else // If we have seen main before
        {
            mainRedeclLine = line; // Record the new line as a redeclaration/redefinition
        }
        if (!(returnType == "int" && paramTypes.empty())) // Check main has signature: int main()
        {
            mainSignatureValid = false; // Mark signature as invalid
        }
    }

    return true; // Function declaration/definition recorded successfully
}

void Semantic::enterFunctionBody(const string &name) // Start tracking inside a function body
{
    currentFunction = name; // Remember which function we are in
    enterScope();           // function-local scope (parameters + locals) // Create a new scope for function locals and parameters
}

void Semantic::leaveFunctionBody() // Stop tracking inside the function body
{
    leaveScope();            // Leave the function's local scope
    currentFunction.clear(); // Clear the current function name
}

void Semantic::noteReturn(int line, const string &returnedType) // Record a return statement and check types
{
    if (currentFunction.empty())
        return;                                   // If not inside a function, do nothing
    FunctionInfo &f = functions[currentFunction]; // Get info for the current function
    f.hasReturnStatement = true;                  // Mark that we saw a return
    if (f.returnType == "void")                   // If function returns void
    {
        logError("Return with a value in void function '" + f.name + "'", line); // Error if value returned in void
        hasSemanticError = true;                                                 // Mark semantic error
    }
    else if (!returnedType.empty() && returnedType != f.returnType) // If returned type does not match function return type
    {
        logError("Type mismatch in return statement of function '" + f.name + "'", line); // Report mismatch
        hasSemanticError = true;                                                          // Mark error
    }
}

void Semantic::logInfo(const string &message, int line) // Save an informational message
{
    infoLogs.push_back("Info: " + message + " at " + fileName + ":" + to_string(line)); // Add message with file and line
}

void Semantic::logWarning(const string &message, int line) // Save a warning message
{
    warningLogs.push_back("Warning: " + message + " at " + fileName + ":" + to_string(line)); // Add warning with file and line
}

void Semantic::logError(const string &message, int line) // Save an error message
{
    errorLogs.push_back("Semantic Error: " + message + " at " + fileName + ":" + to_string(line)); // Add error with file and line
    hasSemanticError = true;                                                                       // Mark that at least one semantic error happened
}

void Semantic::logDeclarationParsed(const string &name, const string &typeRepr, int line, const string &initializer) // Log details of a declaration
{
    string msg = "Declaration parsed: " + name + " (type=" + typeRepr + ")"; // Build a base message
    if (!initializer.empty())                                                // If there was an initializer
    {
        msg += " initializer=" + initializer; // Add initializer info
    }
    logInfo(msg, line); // Record as info
}

void Semantic::logAssignment(const string &name, const string &exprRepr, int line) // Log an assignment action
{
    logInfo("Assignment: " + name + " = " + exprRepr, line); // Save a message that shows what was assigned
}

void Semantic::logIfStatement(int line) // Log that we parsed an if-else
{
    logInfo("If-else statement parsed", line); // Save info message
}

void Semantic::logForLoop(int line) // Log that we parsed a for loop
{
    logInfo("For loop parsed", line); // Save info message
}

void Semantic::logReturn(int line) // Log that we parsed a return statement
{
    logInfo("Return statement parsed", line); // Save info message
}

void Semantic::logFunctionCall(const string &name, int line) // Log that a function was called
{
    logInfo("Function call: " + name, line); // Save info message with function name
}

string Semantic::getVariableType(const string &name, int line) // Ask for a variable's type by name
{
    SymbolInfo *sym = lookup(name); // Find the symbol
    if (!sym)                       // If not found
    {
        logError("Use of undeclared identifier '" + name + "'", line); // Report error for missing declaration
        hasSemanticError = true;                                       // Mark error
        return "";                                                     // Return empty type
    }
    return sym->type; // Return the type string (like "int")
}

void Semantic::checkMainConstraints() // Check rules for the main function
{
    if (mainSeen && mainRedeclLine != 0) // If main was seen more than once
    {
        errorLogs.push_back(
            "Semantic Error: Multiple definitions of function 'main' (first defined at " +
            fileName + ":" + to_string(mainFirstLine) + ", redefined at " +
            fileName + ":" + to_string(mainRedeclLine) + ")."); // Add a detailed error message
        hasSemanticError = true;                                // Mark error
    }
    if (mainSeen && !mainSignatureValid) // If main has wrong signature
    {
        errorLogs.push_back(
            "Semantic Error: 'main' must have signature 'int main()' at " +
            fileName + ":" + to_string(mainRedeclLine ? mainRedeclLine : mainFirstLine)); // Add an error about signature
        hasSemanticError = true;                                                          // Mark error
    }
}

void Semantic::finalize() // Finish semantic analysis and print all messages
{
    // Check missing returns for non-void functions // Warn if needed
    for (auto &p : functions) // Go through all recorded functions
    {
        FunctionInfo &f = p.second;                                             // Get the function info
        if (f.returnType != "void" && f.hasDefinition && !f.hasReturnStatement) // If non-void, defined, and no return found
        {
            warningLogs.push_back(
                "Warning: Control may reach end of non-void function '" + f.name + "' without a return statement at " +
                fileName + ":" + to_string(f.line)); // Add a warning message
        }
    }

    checkMainConstraints(); // Run checks specific to main

    // Print logs in order: Infos, Warnings, Errors // Output messages for the user
    for (const auto &s : infoLogs)
        std::cout << s << std::endl; // Print all info messages
    for (const auto &s : warningLogs)
        std::cout << s << std::endl; // Print all warnings
    for (const auto &s : errorLogs)
        std::cout << s << std::endl; // Print all errors

    if (hasSemanticError) // If there were semantic errors
    {
        // In a fuller compiler we might throw/exit; here we just report. // We could stop the build here, but we only report
    }
}

bool Semantic::checkFunctionCall(const string& name,
                                 const vector<string>& argTypes,
                                 int line)
{
    auto it = functions.find(name);
    if (it == functions.end()) {
        logError("Call to undeclared function '" + name + "'", line);
        hasSemanticError = true;
        return false;
    }
    FunctionInfo& f = it->second;
    if (f.paramTypes.size() != argTypes.size()) {
        logError("Argument count mismatch in call to '" + name +
                 "' (expected " + to_string(f.paramTypes.size()) +
                 ", got " + to_string(argTypes.size()) + ")", line);
        hasSemanticError = true;
        return false;
    }
    for (size_t i = 0; i < argTypes.size(); ++i) {
        if (argTypes[i] != f.paramTypes[i]) {
            logError("Argument type mismatch in call to '" + name +
                     "' at position " + to_string(i + 1) +
                     " (expected " + f.paramTypes[i] +
                     ", got " + argTypes[i] + ")", line);
            hasSemanticError = true;
            return false;
        }
    }
    return true;
}

string Semantic::getFunctionReturnType(const string& name, int line)
{
    auto it = functions.find(name);
    if (it == functions.end()) {
        logError("Use of undeclared function '" + name + "'", line);
        hasSemanticError = true;
        return "";
    }
    return it->second.returnType;
}

void Semantic::checkAssignmentType(const string& varName, const string& rhsType, int line)
{
    SymbolInfo* sym = lookup(varName); // Find the variable
    if (!sym) {
        logError("Use of undeclared identifier '" + varName + "'", line);
        hasSemanticError = true;
        return;
    }
    if (sym->isArray) {
        logError("Cannot assign to array variable '" + varName + "'", line);
        hasSemanticError = true;
        return;
    }
    if (rhsType != sym->type) { // Check types match
        logError("Type mismatch in assignment to '" + varName +
                 "' (expected " + sym->type + ", got " + rhsType + ")", line);
        hasSemanticError = true;
    }
}

void Semantic::checkArrayElementAssignment(const string& varName, 
                                           const string& elementType, 
                                           int line)
{
    SymbolInfo* sym = lookup(varName); // Find the array variable
    if (!sym) {
        logError("Use of undeclared identifier '" + varName + "'", line);
        hasSemanticError = true;
        return;
    }
    if (!sym->isArray) {
        logError("Cannot index non-array variable '" + varName + "'", line);
        hasSemanticError = true;
        return;
    }
    // Check the assigned value matches the array's element type
    if (elementType != sym->type) {
        logError("Type mismatch in array element assignment to '" + varName +
                 "' (expected " + sym->type + ", got " + elementType + ")", line);
        hasSemanticError = true;
    }
}
