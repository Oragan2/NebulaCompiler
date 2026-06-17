#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <vector>
#include <string>

enum class TokenType {
    RETURN, //done
    INT_SIGNED_32, //done
    SEMICOLON, //done
    PLUS, //done
    MINUS, //done
    STAR, //done
    SLASH, //done
    PERCENT, //done
    AND, //done
    OR, //done
    XOR, //done
    NOT, //done
    EXCLAMATION, //done
    SHIFTL, //done
    SHIFTR, //done
    EOFTOKEN
};

struct Token {
    TokenType type;
    std::string val;
    unsigned int column;
    unsigned int line;

    Token(TokenType token, std::string value, unsigned int col, unsigned int li) : type{token}, val{value} {}
    Token(TokenType token, unsigned int col, unsigned int li) : Token{token, "", col, li} {}
}; 

enum class State { START, TEXT, NUMBER };

std::vector<Token> lexer(std::ifstream& file);
std::ostream& operator<<(std::ostream& os, TokenType token); 
#endif
