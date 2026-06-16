#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>
#include <vector>
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens{tokens}, cursor{0} {}

inline Token Parser::peek() { return tokens[cursor]; }

Token Parser::consume(TokenType expected) {
    if (expected == peek().type) {
        return tokens[cursor++];
    }
    std::cerr << expected << " : " << peek().type << std::endl;
    throw std::runtime_error("Syntax Error "); // will make a separate function later
}

ASTNode Parser::parse_sentence() {
    if (peek().type == TokenType::RETURN)
        return parse_return_sentence();
    else
        throw std::runtime_error("Unknown sentence type");
}

ASTNode Parser::parse_return_sentence() {
    consume(TokenType::RETURN);
    ASTNode expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);

    return ReturnStmtNode{std::make_unique<ASTNode>(std::move(expr))};
}

ASTNode Parser::parse_primary() {
    return (IntLiteralNode){.value = consume(TokenType::INT_SIGNED_32).val};
}

Precedence get_token_precedence(TokenType type) {
    switch(type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
            return Precedence::SUM;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return Precedence::PRODUCT;
        case TokenType::SHIFTL:
        case TokenType::SHIFTR:
            return Precedence::SHIFT;
        case TokenType::EXCLAMATION:
        case TokenType::NOT:
            return Precedence::PREFIX;
        case TokenType::OR:
            return Precedence::BIT_OR;
        case TokenType::AND:
            return Precedence::BIT_AND;
        case TokenType::XOR:
            return Precedence::BIT_XOR;
        default:
            return Precedence::LOWEST;
    }
}

ASTNode Parser::parse_expression(int precedence) {
    ASTNode left = parse_primary();

    while (precedence < get_token_precedence(peek().type)) {
        Token op = peek();
        consume(op.type);

        ASTNode right = parse_expression(get_token_precedence(op.type));

        left = BinaryOpNode{op.type,std::make_unique<ASTNode>(std::move(left)),std::make_unique<ASTNode>(std::move(right))};
    }

    return left;
}

std::vector<ASTNode> Parser::parse() {
    std::vector<ASTNode> astnodes;
    while (cursor < tokens.size()) {
        Token token = tokens[cursor];
        astnodes.push_back(parse_sentence());
    }
    return astnodes;
}
