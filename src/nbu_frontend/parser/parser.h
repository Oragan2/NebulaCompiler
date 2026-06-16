#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include <vector>
#include <variant>
#include <memory>

using ASTNode = std::variant<
    struct IntLiteralNode,
    struct ReturnStmtNode,
    struct BinaryOpNode
>;

struct IntLiteralNode {
    int value;
};

struct ReturnStmtNode {
    std::unique_ptr<ASTNode> expression;
};

struct BinaryOpNode {
    TokenType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
};

enum Precedence {
    LOWEST,
    LOGICAL_OR,
    LOGICAL_AND,
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
    ASTNode parse_expression(int precedence);
    ASTNode parse_primary();
};

#endif