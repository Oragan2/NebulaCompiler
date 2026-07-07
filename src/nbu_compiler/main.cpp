#include "../../nbu_frontend/parser/parser.h"
#include "../../nbu_frontend/lexer/lexer.h"
#include "../../nbu_frontend/semanticAnalyser/semantic.h"
#include "ir.h"
#include "type.h"
#include "irTranslator.h"
#include <cstdlib>
#include <fstream>
#include <vector>
#include <variant>
#include <iostream>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

void print_node(const nbuFrontend::ASTNode& node) {
    std::visit(overloads {
            [](const nbuFrontend::Int32LiteralNode& n) {
                std::cout << "Int " << n.value << " ";
            },
            [](const nbuFrontend::BinaryOpNode& n) {
                std::cout << "BinaryOp(";
                print_node(*n.left);
                std::cout << " " <<  n.op << " ";
                print_node(*n.right);
                std::cout << ") ";
            },
            [](const nbuFrontend::ReturnStmtNode& n) {
                std::cout << "Return ";
                print_node(*n.expression);
            },
            [](const nbuFrontend::VariableDeclareNode& n) {
                std::cout << "VariableDeclare " << n.name << " " << n.type << " ";
                if (n.info != nullptr)
                    print_node(*n.info);
            },
            [](const nbuFrontend::VariableAccessNode& n) {
                std::cout << "VariableAccess " << n.name;
            },
            [](const nbuFrontend::UnaryOpNode& n) {
                std::cout << "UnaryOpNode(" << n.op;
                print_node(*n.operand);
                std::cout << ") ";
            },
            [](const nbuFrontend::IfStmtNode& n) {
                std::cout << "if (";
                print_node(*n.condition);
                std::cout << ") ";
                print_node(*n.ifNode);
                if (n.elseNode != nullptr) {
                    std::cout << "\nelse ";
                    print_node(*n.elseNode);
                }
            },
            [](const nbuFrontend::BlockStmtNode& n) {
                std::cout << "{\n";
                for (const auto& sentence : n.codes) {
                    std::cout << "\t";
                    print_node(*sentence);
                    std::cout << "\n";
                }
                std::cout << "}";
            },
            [](const nbuFrontend::FuncStmtNode& n) {
                std::cout << "func " << n.retType << "(";
                for(const auto& m : n.parameters)
                    print_node(*m);
                std::cout << ") ";
                print_node(*n.code);
            },
            [](const nbuFrontend::Float32LiteralNode& n) {
                std::cout << "Float " << n.value << " ";
            },
            [](const nbuFrontend::FuncCallStmtNode& n) {
                std::cout << "Function call " << n.name << "(";
                for (const auto& param : n.callParameters) {
                    print_node(*param);
                    std::cout << "";
                }
                std::cout << ")";
            },
            [](const nbuFrontend::VariableModNode& n) {
                std::cout << "VariableModification ";
                print_node(*n.variable);
                std::cout << " ";
                print_node(*n.info);
            },
            [](const nbuFrontend::PromotionNode& n) {
                std::cout << "Promotion from a " << n.was << " to a " << n.topromote << " ";
                print_node(*n.info);
            },
            [](const nbuFrontend::readAddrNode& n) {
                std::cout << "Read " << n.quantity/8 << " bytes from ";
                print_node(*n.addr);
            },
            [](const nbuFrontend::writeAddrNode& n) {
                std::cout << "Write " << n.quantity/8 << " bytes from ";
                print_node(*n.addr);
                std::cout << " as ";
                print_node(*n.value);
            },
            [](const nbuFrontend::asmNode& n) {
                std::cout << "asm {" << n.rawAsm << "}";
            },
            [](const nbuFrontend::EnumDeclNode& n) {
                std::cout << "enum " << n.name << "{\n\t";
                for (const auto&[member,number] : n.members) {
                    std::cout << member << " = " << number << ",\n\t";
                }
                std::cout << "}";
            },
            [](const nbuFrontend::EnumAccessNode& n) {
                std::cout << n.enumName << "::" << n.enumMember;
            },
            [](const nbuFrontend::StructDeclNode& n) {
                std::cout << "struct " << n.structName << "{\n\t";
                for (const auto& field : n.fields) {
                    std::cout << field.first << " " << field.second;
                    std::cout << "\n\t";
                }
                std::cout << "}";
            },
            [](const nbuFrontend::StructAccessNode& n) {
                std::cout << "StructAccess ";
                print_node(*n.firstPart);
                std::cout << " field " << n.fieldName;
            }
            }, node);
}

void print_tree(const std::vector<nbuFrontend::ASTNode>& astnodes) {
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

  std::vector<nbuFrontend::Token> tokens = nbuFrontend::lexer(inputfile);

  nbuFrontend::Parser parser(tokens);

  std::vector<nbuFrontend::ASTNode> nodes = parser.parse();

  nbuFrontend::Semantic semantic(nodes);

  auto [errors, warnings] = semantic.semanticAnalyses();

  std::cout << "The analyses ended with " << errors << " errors and " << warnings << " warnings" << std::endl;

  if (errors != 0) {
    std::cerr << "Fix errors before recompilling" << std::endl;
    return EXIT_FAILURE;
  }

  //print_tree(semantic.getNodes());
   
  nbuIR::IRTranslator irtranslator{semantic.getNodes(), semantic.getEnums()};

  auto IR = irtranslator.Translate();
  
  std::cout << nbuIR::irProgramToStr(IR);

  std::ofstream outputFile("a.out");

  return EXIT_SUCCESS;
}
