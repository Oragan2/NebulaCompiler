#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <vector>

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

    Token(TokenType token, int value) : type{token}, val{value} {}
    Token(TokenType token) : Token{token, 0} {}
}; 

enum class State { START, TEXT, NUMBER };

std::vector<Token> lexer(std::ifstream& file);
std::ostream& operator<<(std::ostream& os, TokenType token); 
#endif
