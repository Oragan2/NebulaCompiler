#ifndef LEXER_HPP
#define LEXER_HPP

enum class TokenType {
    RETURN,
    INT_SIGNED_32,
    SEMICOLON,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    AND,
    OR,
    XOR,
    NOT,
    EXCLAMATION,
    SHIFTL,
    SHIFTR
};

struct Token {
    TokenType type;
    int val;
}; 

#endif