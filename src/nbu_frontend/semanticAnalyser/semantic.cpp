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

namespace nbuFrontend {
    std::unordered_set<TokenType> compOp = {
        TokenType::EQUALEQUAL,
        TokenType::MT,
        TokenType::MTE,
        TokenType::LT,
        TokenType::LTE,
        TokenType::DIFFERENT
    };

    Semantic::Semantic(std::vector<ASTNode>& nodes) : nodes{nodes} {}

    std::vector<ASTNode>& Semantic::getNodes() {
        return nodes;
    }

    std::pair<int, int> Semantic::semanticAnalyses() {
        for (ASTNode& node : nodes) {
            std::visit(overloads {
                [this](VariableDeclare& n) {
                    GlobalSymboleTable.emplace(n.name, SymboleInfo{.type = n.type, .stack_offset = 0});
                    if (n.info != nullptr) {
                        codeSemanticAnalyses(*n.info);
                        Type retType = type_precision(*n.info);
                        if (retType != n.type) {
                            Type promote = tryPromote(retType, n.type);
                            if (promote == retType) {
                                print_error("The variable is a "+n.type+" but tryed to declare it a "+promote);
                            }
                            n.info = std::make_unique<ASTNode>(PromotionNode{promote, retType, std::move(n.info)});
                        }
                    }
                },
                [this](FuncStmtNode& n) {
                    currentFunc = FunctionInfo{.name = n.name, .retType = n.retType};
                    scopeStack.push_back({});
                    for (std::unique_ptr<ASTNode>& parameterNode : n.parameters) {
                        std::visit(overloads {
                            [this](VariableDeclare& n) {
                                currentFunc.paramType.emplace_back(n.type);
                                scopeStack.back().emplace(n.name, SymboleInfo{n.name, n.type, 0});
                                if (n.info != nullptr) {
                                    codeSemanticAnalyses(*n.info);
                                    Type retType = type_precision(*n.info);
                                    if (retType != n.type) {
                                        Type promote = tryPromote(retType, n.type);
                                        if (promote == retType) {
                                            print_error("The variable is a "+n.type+" but tryed to declare it a "+promote);
                                        }
                                        n.info = std::make_unique<ASTNode>(PromotionNode{promote, retType, std::move(n.info)});
                                    }
                                }
                            },
                            [this](auto& n) {print_error("only variable declaration can be done in the function declaration"); }
                        },*parameterNode);
                    }
                    functions.emplace(currentFunc.name, currentFunc);
                    codeSemanticAnalyses(*n.code);
                    scopeStack.pop_back();
                },
                [this](const auto& n) {
                    print_error("only variables and function declare at file root"); // fall back incase the parser didn't already took care of it
                }
            }, node);
        }
            

        for (const std::string& funcName : unknownFunctionName) {
            if (!functions.contains(funcName))
                print_error("Funcion "+funcName+" doesn't exist");
        }

        return {errorNumber, warningNumber};
    }

    void Semantic::codeSemanticAnalyses(ASTNode& node) {
        std::visit(overloads {
            [this](Int32LiteralNode& n) {},
            [this](Float32LiteralNode& n) {},
            [this](ReturnStmtNode& n) {
                codeSemanticAnalyses(*n.expression);
                Type retVal = type_precision(*n.expression);
                if (retVal != currentFunc.retType) {
                    Type promoted = tryPromote(retVal, currentFunc.retType);
                    if (promoted == retVal) {
                        print_error("function returned a different type from the announced value, returns : "+promoted+" instead of a "+currentFunc.retType);
                    }
                    n.expression = std::make_unique<ASTNode>(PromotionNode{promoted, retVal, std::move(n.expression)});
                }
            },
            [this](BinaryOpNode& n) {
                codeSemanticAnalyses(*n.left);
                codeSemanticAnalyses(*n.right);
                if (compOp.contains(n.op)) {
                    Type left = type_precision(*n.left);
                    Type right = type_precision(*n.right);
                    if (left != right) {
                        Type promote = tryPromote(left,right);
                        if (promote != right) {
                            promote = tryPromote(right, left);
                            if (promote != left) {
                                print_error("Can't compare a "+left+" to a "+right);
                            }
                            n.right = std::make_unique<ASTNode>(PromotionNode{right,promote,std::move(n.right)});
                        }
                        n.left = std::make_unique<ASTNode>(PromotionNode{promote,left,std::move(n.left)});
                    }
                }
                else {
                    n.precision = resolve_type(type_precision(*n.left), type_precision(*n.right)); // didn't find another way
                }
            },
            [this](VariableDeclare& n) {
                if (scopeStack.back().contains(n.name)) {
                    print_error("Variable "+n.name+" was already declared somewhere else");
                }
                scopeStack.back().emplace(n.name, SymboleInfo{n.name, n.type, 0});
                if (n.info != nullptr) {
                    codeSemanticAnalyses(*n.info);
                    Type retType = type_precision(*n.info);
                    if (retType != n.type) {
                        Type promote = tryPromote(retType, n.type);
                        if (promote == retType) {
                            print_error("The variable is a "+n.type+" but tryed to declare it a "+promote);
                        }
                        n.info = std::make_unique<ASTNode>(PromotionNode{promote, retType, std::move(n.info)});
                    }
                }
            },
            [this](VariableAccess& n) {
                char found = 0;
                for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                    if (it->contains(n.name))
                        found = 1;
                }
                if (!found) {
                    print_error("Variable "+n.name+" not declared inside this scope");
                }
            },
            [this](UnaryOpNode& n) {
                codeSemanticAnalyses(*n.operand);
                if (n.op == TokenType::EXCLAMATION) {
                    Type typePrecision = type_precision(*n.operand);
                    if (type_precision(*n.operand) != Type{.kind = Type::Kind::INT32}) {
                        Type promote = tryPromote(typePrecision, Type{.kind = Type::Kind::INT32});
                        if (promote == typePrecision)
                            print_error("Can't use a ! other then for boolean expression");
                        n.operand = std::make_unique<ASTNode>(PromotionNode{promote, typePrecision, std::move(n.operand)});
                    }
                }
                else
                    n.precision = type_precision(*n.operand);
            },
            [this](IfStmtNode& n) {
                codeSemanticAnalyses(*n.condition);
                Type condType = type_precision(*n.condition);
                if (condType != Type{.kind = Type::Kind::INT32}) {
                    Type promote = tryPromote(condType, Type{.kind = Type::Kind::INT32});
                    if (promote == condType) {
                        print_error("The value of the condition must be a boolean not a "+condType);
                    }
                    n.condition = std::make_unique<ASTNode>(PromotionNode{promote, condType, std::move(n.condition)});
                }
                codeSemanticAnalyses(*n.ifNode);
                if (n.elseNode != nullptr)
                    codeSemanticAnalyses(*n.elseNode);
            },
            [this](BlockStmtNode& n) {
                scopeStack.push_back({});
                for (std::unique_ptr<ASTNode>& blockNode : n.codes) {
                    codeSemanticAnalyses(*blockNode);
                }
                scopeStack.pop_back();
            },
            [this](FuncCallStmtNode& n) {
                if (!functions.contains(n.name)) {
                    unknownFunctionName.push_back(n.name);
                }
                FunctionInfo func = functions[n.name];
                if (n.callParameters.size() != func.paramType.size()) {
                    print_error("The function "+func.name+" only take "+std::to_string(func.paramType.size())+" arguments but "+std::to_string(n.callParameters.size())+" where given");
                }
                for (unsigned int i = 0; i < func.paramType.size() && i < n.callParameters.size(); ++i) {
                    const std::unique_ptr<ASTNode>& param = n.callParameters[i];
                    Type& type = func.paramType[i];
                    Type paramType = type_precision(*param);
                    if (type != paramType) {
                        Type promote = tryPromote(paramType, type);
                        if (promote == paramType) {
                            print_error("Expected argument type "+type+" but received a "+promote+" for parameter "+std::to_string(i));
                        }
                        n.callParameters[i] = std::make_unique<ASTNode>(PromotionNode{promote, paramType, std::move(n.callParameters[i])});
                    } 
                }
            },
            [this](VariableModNode& n) {
                if (!(scopeStack.back().contains(n.name) || GlobalSymboleTable.contains(n.name))) 
                    print_error("The variable "+n.name+" doesn't exist");
                codeSemanticAnalyses(*n.info);
                Type retType = type_precision(*n.info);
                SymboleInfo var = scopeStack.back().contains(n.name) ? scopeStack.back()[n.name] : GlobalSymboleTable[n.name];
                if (retType != var.type) {
                    Type promote = tryPromote(retType, var.type);
                    if (promote == retType) {
                        print_error("The variable is a "+var.type+" but tryed to asigne a "+promote+" to it");
                    }
                    n.info = std::make_unique<ASTNode>(PromotionNode{promote, retType, std::move(n.info)});
                }
            },
            [this](readAddrNode& n) {
                codeSemanticAnalyses(*n.addr);
                Type addrType = type_precision(*n.addr);
                if (addrType != Type{.kind = Type::Kind::VADDR} && addrType != Type{.kind = Type::Kind::PADDR}) {
                    print_error("The read function can only take an addresse and not a "+addrType);
                }
            },
            [this](writeAddrNode& n) {
                codeSemanticAnalyses(*n.addr);
                codeSemanticAnalyses(*n.value);
                Type addrType = type_precision(*n.addr);
                if (addrType != Type{.kind = Type::Kind::VADDR} && addrType != Type{.kind = Type::Kind::PADDR}) {
                    print_error("The read function can only take an addresse and not a "+addrType);
                }
            },
            [this](asmNode& n) {
                //do nothing
            },
            [this](PromotionNode& n) {
                codeSemanticAnalyses(*n.info);
            },
            [this](FuncStmtNode& n) {
                print_error("Function declaration impossible inside another function");
            }
        }, node);
    }

    Type Semantic::type_precision(const ASTNode& node) {
        return std::visit(overloads {
            [this](const Int32LiteralNode& n) {return Type{.kind = Type::Kind::INT32};},
            [this](const Float32LiteralNode& n) {return Type{.kind = Type::Kind::FLOAT32};},
            [this](const VariableAccess& n) {
                for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                    if (it->contains(n.name))
                        return (*it)[n.name].type;
                }
                if (GlobalSymboleTable.contains(n.name))
                    return GlobalSymboleTable.at(n.name).type;
                return Type{.kind = Type::Kind::INT32};
            },
            [this](const BinaryOpNode& n) {return resolve_type(type_precision(*n.left), type_precision(*n.right));},
            [this](const UnaryOpNode& n) {return type_precision(*n.operand);},
            [this](const FuncCallStmtNode& n) {return functions.at(n.name).retType;},
            [this](const readAddrNode& n ) {
                if (n.quantity == 64) return Type{.kind = Type::Kind::UINT64};
                if (n.quantity == 32) return Type{.kind = Type::Kind::UINT32};
                if (n.quantity == 16) return Type{.kind = Type::Kind::UINT32}; // will change
                else return Type{.kind = Type::Kind::UINT32}; // will change
            },
            [this](const auto&) {print_error("Uh?"); return Type{.kind = Type::Kind::INT32};}
        }, node);
    }

    Type Semantic::resolve_type(Type left, Type right) {
        if (left == right) return left;
        if (left == Type{.kind = Type::Kind::FLOAT64} || right == Type{.kind = Type::Kind::FLOAT64}) return Type{.kind = Type::Kind::FLOAT64}; 
        if (left == Type{.kind = Type::Kind::FLOAT32} || right == Type{.kind = Type::Kind::FLOAT32}) return Type{.kind = Type::Kind::FLOAT32}; 
        if (left == Type{.kind = Type::Kind::VADDR} || right == Type{.kind = Type::Kind::VADDR}) return Type{.kind = Type::Kind::VADDR};
        if (left == Type{.kind = Type::Kind::PADDR} || right == Type{.kind = Type::Kind::PADDR}) return Type{.kind = Type::Kind::PADDR};
        if (left == Type{.kind = Type::Kind::UINT64} || right == Type{.kind = Type::Kind::UINT64}) return Type{.kind = Type::Kind::UINT64}; 
        if (left == Type{.kind = Type::Kind::INT64} || right == Type{.kind = Type::Kind::INT64}) return Type{.kind = Type::Kind::INT64}; 
        if (left == Type{.kind = Type::Kind::UINT32} || right == Type{.kind = Type::Kind::UINT32}) return Type{.kind = Type::Kind::UINT32}; 
        return Type{.kind = Type::Kind::INT32};
    }

    Type Semantic::tryPromote(Type currentType, Type promoteTo) {
        if (resolve_type(currentType, promoteTo) == promoteTo)
            return promoteTo;
        if (currentType == Type{.kind = Type::Kind::UINT32} && promoteTo == Type{.kind = Type::Kind::INT32}) return promoteTo;
        if (currentType == Type{.kind = Type::Kind::UINT64} && promoteTo == Type{.kind = Type::Kind::INT64}) return promoteTo;
        if (currentType == Type{.kind = Type::Kind::INT64} && promoteTo == Type{.kind = Type::Kind::INT32}) return promoteTo;
        if (currentType == Type{.kind = Type::Kind::UINT64} && promoteTo == Type{.kind = Type::Kind::UINT32}) return promoteTo;
        if (currentType == Type{.kind = Type::Kind::UINT64} && promoteTo == Type{.kind = Type::Kind::INT32}) return promoteTo;
        if (currentType == Type{.kind = Type::Kind::INT64} && promoteTo == Type{.kind = Type::Kind::UINT32}) return promoteTo;
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
}