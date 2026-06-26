#include "../nbu_frontend/parser/parser.h"
#include "../nbu_frontend/lexer/lexer.h"
#include "semantic.h"
#include <cstdlib>
#include <vector>
#include <variant>
#include <iostream>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

void print_node(const ASTNode& node) {
    std::visit(overloads {
            [](const Int32LiteralNode& n) {
                std::cout << "Int " << n.value << " ";
            },
            [](const BinaryOpNode& n) {
                std::cout << "BinaryOp(";
                print_node(*n.left);
                std::cout << n.op << " ";
                print_node(*n.right);
                std::cout << ") ";
            },
            [](const ReturnStmtNode& n) {
                std::cout << "Return ";
                print_node(*n.expression);
            },
            [](const VariableDeclare& n) {
                std::cout << "VariableDeclare " << n.type << " ";
                if (n.info != nullptr)
                    print_node(*n.info);
            },
            [](const VariableAccess& n) {
                std::cout << "VariableAccess " << n.name;
            },
            [](const UnaryOpNode& n) {
                std::cout << "UnaryOpNode(" << n.op;
                print_node(*n.operand);
                std::cout << ") ";
            },
            [](const IfStmtNode& n) {
                std::cout << "if (";
                print_node(*n.condition);
                std::cout << ") ";
                print_node(*n.ifNode);
                if (n.elseNode != nullptr) {
                    std::cout << "\nelse ";
                    print_node(*n.elseNode);
                }
            },
            [](const BlockStmtNode& n) {
                std::cout << "{\n";
                for (const auto& sentence : n.codes) {
                    std::cout << "\t";
                    print_node(*sentence);
                    std::cout << "\n";
                }
                std::cout << "}";
            },
            [](const FuncStmtNode& n) {
                std::cout << "func " << n.retType << "(";
                for(const auto& m : n.parameters)
                    print_node(*m);
                std::cout << ") ";
                print_node(*n.code);
            },
            [](const Float32LiteralNode& n) {
                std::cout << "Float " << n.value << " ";
            },
            [](const FuncCallStmtNode& n) {
                std::cout << "Function call " << n.name << "(";
                for (const auto& param : n.callParameters) {
                    print_node(*param);
                    std::cout << "";
                }
                std::cout << ")";
            },
            [](const VariableModNode& n) {
                std::cout << "VariableModification " << n.name << " ";
                print_node(*n.info);
            },
            [](const PromotionNode& n) {
                std::cout << "Promotion from a " << n.was << " to a " << n.topromote << " ";
                print_node(*n.info);
            },
            [](const readAddrNode& n) {
                std::cout << "Read " << n.quantity/8 << " bytes from ";
                print_node(*n.addr);
            },
            [](const writeAddrNode& n) {
                std::cout << "Write " << n.quantity/8 << " bytes from ";
                print_node(*n.addr);
                std::cout << " as ";
                print_node(*n.value);
            },
            [](const asmNode& n) {
                std::cout << "asm {" << n.rawAsm << "}";
            }
            }, node);
}

void print_tree(const std::vector<ASTNode>& astnodes) {
    for (const auto& astnode : astnodes) {
        print_node(astnode);
        std::cout << "\n";
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

  Semantic semantic(nodes);

  auto [errors, warnings] = semantic.semanticAnalyses();

  std::cout << "The analyses ended with " << errors << "errors and " << warnings << " warnings" << std::endl;

  if (errors == 0)
    print_tree(semantic.getNodes());
  else {
    std::cerr << "Fix errors before recompilling" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
