#include "../nbu_frontend/parser/parser.h"
#include "../nbu_frontend/lexer/lexer.h"
#include <vector>
#include <variant>
#include <iostream>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

void print_node(const ASTNode& node) {
    std::visit(overloads {
            [](const IntLiteralNode& n) {
                std::cout << "Int " << n.value << " ";
            },
            [](const BinaryOpNode& n) {
                std::cout << "BinaryOp ";
                print_node(*n.left);
                print_node(*n.right);
            },
            [](const ReturnStmtNode& n) {
                std::cout << "Return ";
                print_node(*n.expression);
            }
        }, node);
}

void print_tree(const std::vector<ASTNode>& astnodes) {
    for (const auto& astnode : astnodes) {
        print_node(astnode);
        std::cout << '\n';
    }
}

int main() {
    std::vector<Token> tokens;
    tokens.push_back({.type = TokenType::RETURN, .val = -1});
    tokens.push_back({.type = TokenType::INT_SIGNED_32, .val = 10});
    tokens.push_back({.type = TokenType::PLUS, .val = -1});
    tokens.push_back({.type = TokenType::INT_SIGNED_32, .val = 42});
    tokens.push_back({.type = TokenType::STAR, .val = 42});
    tokens.push_back({.type = TokenType::INT_SIGNED_32, .val = 2});
    tokens.push_back({.type = TokenType::SEMICOLON, .val = -1});
    Parser parser(tokens);
    auto astnodes = parser.parse();
    print_tree(astnodes);
    return 0;
}