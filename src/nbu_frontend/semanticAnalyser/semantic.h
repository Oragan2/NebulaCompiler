#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "lexer.h"
#include "parser.h"
#include <vector>
#include <unordered_map>
#include <utility>

struct SymboleInfo {
    std::string name;
    TokenType type;
    unsigned int stack_offset;
};

struct FunctionInfo {
    std::string name;
    TokenType retType;
    std::vector<TokenType> paramType;
};

class Semantic {
    public:
    Semantic(std::vector<ASTNode>& nodes);
    std::pair<int, int> semanticAnalyses();
    std::vector<ASTNode>& getNodes();

    private:
    std::vector<ASTNode>& nodes;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::unordered_map<std::string, SymboleInfo> GlobalSymboleTable;
    std::vector<std::string> unknownFunctionName; //For allowing to call functions declared after it just a final quick check
    std::vector<std::unordered_map<std::string, SymboleInfo>> scopeStack;
    FunctionInfo currentFunc;
    unsigned int errorNumber = 0;
    unsigned int warningNumber = 0;

    void codeSemanticAnalyses(ASTNode& node);
    TokenType type_precision(const ASTNode& node);
    TokenType resolve_type(TokenType left, TokenType right);
    TokenType tryPromote(TokenType currentType, TokenType promoteTo);
    void print_error(const std::string& msg);
    void print_warning(const std::string& msg);
};

#endif