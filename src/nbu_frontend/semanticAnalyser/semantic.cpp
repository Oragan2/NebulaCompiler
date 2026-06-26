#include "semantic.h"
#include "lexer.h"
#include "parser.h"
#include <memory>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <iostream>
#include <string>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

std::unordered_set<TokenType> compOp = {
    TokenType::EQUALEQUAL,
    TokenType::MT,
    TokenType::MTE,
    TokenType::LT,
    TokenType::LTE,
    TokenType::DIFFERENT
};

std::pair<int, int> Semantic::semanticAnalyses() {
    for (const ASTNode& node : nodes)
    std::visit(overloads {
        [this](const VariableDeclare& n) {
            GlobalSymboleTable.emplace(n.name, SymboleInfo{.type = n.type, .stack_offset = 0});
        },
        [this](const FuncStmtNode& n) {
            currentFunc = FunctionInfo{.name = n.name, .retType = n.retType};
            for (const std::unique_ptr<ASTNode>& parameterNode : n.parameters)
                std::visit(overloads {
                    [this](const VariableDeclare& n) {currentFunc.paramType.emplace_back(n.type);},
                    [this](const auto& n) {print_error("only variable declaration can be done in the function declaration");}
                },*parameterNode);
            functions.emplace(currentFunc.name, currentFunc);
        },
        [this](const auto& n) {
            print_error("only variables and function declare at file root"); // fall back incase the parser didn't already took care of it
        }
    }, node);

    return {errorNumber, warningNumber};
}

void Semantic::codeSemanticAnalyses(const ASTNode& node) {
    std::visit(overloads {
        [this](const Int32LiteralNode& n) {},
        [this](const Float32LiteralNode& n) {},
        [this](const ReturnStmtNode& n) {
            TokenType retVal = type_precision(*n.expression);
            if (retVal != currentFunc.retType) {
                TokenType promoted = tryPromote(retVal, promoted);
                if (promoted == retVal) {
                    print_error("function returned a different type from the announced value, returns : "+promoted+" instead of a "+currentFunc.retType);
                }
            }
        },
        [this](BinaryOpNode& n) {
            if (compOp.contains(n.op)) {
                n.precision = TokenType::INT32;
                TokenType left = type_precision(*n.left);
                TokenType right = type_precision(*n.right);
                if (left != right) {
                    TokenType promote = tryPromote(left,right);
                    if (promote != right) {
                        promote = tryPromote(right, left);
                        if (promote != left) {
                            print_error("Can't compare a "+left+" to a "+right);
                        }
                    }
                }
            }
            else
                n.precision = resolve_type(type_precision(*n.left), type_precision(*n.right)); // didn't find another way
        }
    }, node);
}

TokenType Semantic::type_precision(const ASTNode& node) {
    return std::visit(overloads {
        [this](const Int32LiteralNode& n) {return TokenType::INT32;},
        [this](const Float32LiteralNode& n) {return TokenType::FLOAT32;},
        [this](const VariableAccess& n) {
            if (currentFunc.LocalSymboleTable.contains(n.name))
                return currentFunc.LocalSymboleTable.at(n.name).type;
            else
                return GlobalSymboleTable.at(n.name).type;
        },
        [this](const BinaryOpNode& n) {return resolve_type(type_precision(*n.left), type_precision(*n.right));},
        [this](const UnaryOpNode& n) {return type_precision(*n.operand);},
        [this](const FuncCallStmtNode& n) {return functions.at(n.name).retType;},
        [this](const readAddrNode& n ) {
            if (n.quantity == 64) return TokenType::UINT64;
            if (n.quantity == 32) return TokenType::UINT32;
            if (n.quantity == 16) return TokenType::UINT32; // will change
            else return TokenType::UINT32; // will change
        },
        [this](const auto&) {print_error("Uh?"); return TokenType::EOFTOKEN;}
    }, node);
}

TokenType Semantic::resolve_type(TokenType left, TokenType right) {
    if (left == right) return left;
    if (left == TokenType::FLOAT64 || right == TokenType::FLOAT64) return TokenType::FLOAT64; 
    if (left == TokenType::FLOAT32 || right == TokenType::FLOAT32) return TokenType::FLOAT32; 
    if (left == TokenType::VADDR || right == TokenType::VADDR) return TokenType::VADDR;
    if (left == TokenType::PADDR || right == TokenType::PADDR) return TokenType::PADDR;
    if (left == TokenType::UINT64 || right == TokenType::UINT64) return TokenType::UINT64; 
    if (left == TokenType::INT64 || right == TokenType::INT64) return TokenType::INT64; 
    if (left == TokenType::UINT32 || right == TokenType::UINT32) return TokenType::UINT32; 
    return TokenType::INT32;
}

TokenType Semantic::tryPromote(TokenType currentType, TokenType promoteTo) {
    if (resolve_type(currentType, promoteTo) == promoteTo)
        return promoteTo;
    if (currentType == TokenType::UINT32 && promoteTo == TokenType::INT32) return promoteTo;
    if (currentType == TokenType::UINT64 && promoteTo == TokenType::INT64) return promoteTo;
    if (currentType == TokenType::INT64 && promoteTo == TokenType::INT32) return promoteTo;
    if (currentType == TokenType::UINT64 && promoteTo == TokenType::UINT32) return promoteTo;
    if (currentType == TokenType::UINT64 && promoteTo == TokenType::INT32) return promoteTo;
    if (currentType == TokenType::INT64 && promoteTo == TokenType::UINT32) return promoteTo;
    return currentType;
}

void Semantic::print_error(const std::string& msg) {
    std::cerr << "Error : " << msg << std::endl;
    ++errorNumber;
}

void Semantic::print_warning(const std::string& msg) {
    std::cerr << "Warning : " << msg << std::endl;
    ++warningNumber;
}