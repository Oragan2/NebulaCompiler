#ifndef LEXER_HPP
#define LEXER_HPP

#include <fstream>
#include <vector>
#include <string>

namespace nbuFrontend {
    // the one marked as done mean they are printable to the console
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
        IDENTIFIER,
        INT32, //done
        UINT32, //done
        EQUAL, //done
        EQUALEQUAL, //done
        DIFFERENT, //done
        LT, //done
        MT, //done
        LTE, //done
        MTE, //done
        ANDAND, //done
        OROR, //done
        XORXOR, //done
        IF, //done
        ELSE, //done
        LPARAM, //done
        RPARAM, //done
        LBRAK, //done
        RBRAK, //done
        COMMA, //done
        FLOAT64, //done
        FLOAT32, //done
        INT64, //done
        UINT64, //done
        FLOAT_SIGNED_32, //done
        VADDR, //done
        PADDR, //done
        ASM, // done
        ASM_INSTRUCTIONS, // done
        EOFTOKEN //done
    };

    struct Token {
        TokenType type;
        std::string val;
        unsigned int column;
        unsigned int line;

        Token(TokenType token, const std::string& value, unsigned int col, unsigned int li) : type{token}, val{std::move(value)}, column{col}, line{li} {}
        Token(TokenType token, unsigned int col, unsigned int li) : Token{token, "", col, li} {}
    }; 

    enum class State { START, TEXT, NUMBER };

    std::vector<Token> lexer(std::ifstream& file);
    std::ostream& operator<<(std::ostream& os, TokenType token); 
    std::string operator+(const std::string& str, TokenType token);
    std::string token_to_str(TokenType token);
}

#endif
