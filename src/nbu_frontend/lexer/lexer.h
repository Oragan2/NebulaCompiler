#ifndef LEXER_HPP
#define LEXER_HPP

enum class TokenType {
    RETURN,
    INT_SIGNED_32,
    SEMICOLON,
    ALL
};

struct Token {
    TokenType type;
    int val;
}; 

#endif