#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <list>
#include <map>

using ASTNode = std::variant<
    struct Int32LiteralNode,
    struct FLoat32LiteralNode,
    struct ReturnStmtNode,
    struct BinaryOpNode,
    struct VariableDeclare,
    struct VariableAccess,
    struct UnaryOpNode,
    struct IfStmtNode,
    struct BlockStmtNode,
    struct FuncStmtNode
>;

struct Int32LiteralNode {
    int32_t value;
};

struct FLoat32LiteralNode {
    float value;
};

struct ReturnStmtNode {
    std::unique_ptr<ASTNode> expression;
};

struct BinaryOpNode {
    TokenType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

struct VariableDeclare {
    TokenType type;
    std::unique_ptr<ASTNode> info;
};

struct VariableAccess {
    std::string name; 
}; //Temp implementation will be changed later

struct UnaryOpNode {
    TokenType op;
    std::unique_ptr<ASTNode> operand;
};

struct IfStmtNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> ifNode;
    std::unique_ptr<ASTNode> elseNode;
};

struct BlockStmtNode {
    std::list<std::unique_ptr<ASTNode>> codes;
};

struct FuncStmtNode {
    TokenType retType;
    std::list<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> code;
};

struct FuncCallStmtNode {
    std::string name;
    std::list<std::unique_ptr<ASTNode>> callParameters;
};

struct VariableModNode {
    std::string name;
    std::unique_ptr<ASTNode> info;
};

struct SymboleInfo {
    TokenType type;
    unsigned int stack_offset;
};

struct FunctionInfo {
    TokenType retType;
    std::map<std::string, SymboleInfo> LocalSymboleTable;
};

enum Precedence {
    LOWEST,
    LOGICAL_OR,
    LOGICAL_AND,
    LOGICAL_XOR,
    BIT_OR,
    BIT_XOR,
    BIT_AND,
    EQUALITY,
    RELATIONAL,
    SHIFT,
    SUM,
    PRODUCT,
    PREFIX
};

class Parser {
    public:
    std::vector<ASTNode> parse();
    Parser(const std::vector<Token>& tokens);
    
    private:
    std::vector<Token> tokens;
    unsigned int cursor;
    std::map<std::string, FunctionInfo> functions;
    std::map<std::string, SymboleInfo> GlobalSymboleTable;
    FunctionInfo currentFunc;

    inline const Token& peek();
    Token consume(TokenType expected);
    ASTNode parse_sentence();
    ASTNode parse_identifier_sentence();
    ASTNode parse_return_sentence();
    ASTNode parse_local_variable_sentence();
    ASTNode parse_global_variable(const std::string& name, TokenType type);
    ASTNode parse_if_sentence();
    ASTNode parse_block();
    ASTNode parse_function(const std::string& name, TokenType retValue);
    ASTNode parse_parameter();
    ASTNode parse_expression(int precedence);
    ASTNode parse_primary();
};

#endif
