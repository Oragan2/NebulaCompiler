#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include <vector>

class Parser {
    public:
    void parse();
    Parser(const std::vector<Token>& tokens);
    private:
    std::vector<Token> tokens;
    unsigned int cursor;

    inline Token peek();
    Token consume(TokenType expected);
};

#endif