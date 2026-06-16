#include "../nbu_frontend/parser/parser.h"
#include "../nbu_frontend/lexer/lexer.h"
#include <vector>

int main() {
    std::vector<Token> tokens;
    tokens.push_back({.type = TokenType::RETURN, .val = -1});
    tokens.push_back({.type = TokenType::INT_SIGNED_32, .val = 0});
    tokens.push_back({.type = TokenType::SEMICOLON, .val = -1});
    Parser parser(tokens);
    parser.parse();
    return 0;
}