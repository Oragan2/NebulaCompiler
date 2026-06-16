#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>
#include <vector>

Parser::Parser(const std::vector<Token>& tokens) : tokens{tokens} {}

inline Token Parser::peek() { return tokens[cursor]; }

Token Parser::consume(TokenType expected) {
    if (expected == peek().type || expected == TokenType::ALL) {
        return tokens[cursor++];
    }
    throw std::runtime_error("Syntax Error"); // will make a separate function later
}

void Parser::parse() {
    TokenType expected{TokenType::RETURN};
    while (cursor < tokens.size()) {
        Token token = tokens[cursor];
        consume(expected);
        switch (token.type) {
            case TokenType::RETURN:
                expected = TokenType::INT_SIGNED_32; // will change to the functions return just for testing
                break;
            case TokenType::INT_SIGNED_32:
                expected = TokenType::SEMICOLON; // will change just for testing purpose
                break;
            case TokenType::SEMICOLON:
                expected = TokenType::ALL;
                break;
        }
    }
}