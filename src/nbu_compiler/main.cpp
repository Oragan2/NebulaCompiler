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

int main(int argc, char **argv) {
  if (argc == 1) {
    throw std::runtime_error("Bad usage, no input file");
  }

  std::ifstream inputfile(argv[1]);

  std::vector<Token> tokens = lexer(inputfile);

  Parser parser(tokens);

  std::vector<ASTNode> nodes = parser.parse();

  print_tree(nodes);

  return EXIT_SUCCESS;
}
