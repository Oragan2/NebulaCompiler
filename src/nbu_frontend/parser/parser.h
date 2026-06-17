#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include <vector>
#include <variant>
#include <memory>
#include <string>

using ASTNode = std::variant<
    struct Int32LiteralNode,
    struct ReturnStmtNode,
    struct BinaryOpNode,
    struct VariableDeclare,
    struct VariableAccess,
    struct UnaryOpNode
>;

struct Int32LiteralNode {
    int32_t value;
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

struct SymboleInfo {
    TokenType type;
    unsigned int stack_offset;
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

    inline Token peek();
    Token consume(TokenType expected);
    ASTNode parse_sentence();
    ASTNode parse_return_sentence();
    ASTNode parse_variable_sentence();
    ASTNode parse_expression(int precedence);
    ASTNode parse_primary();
};

#endif