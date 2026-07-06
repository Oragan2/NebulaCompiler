#include "semantic.h"
#include "type.h"
#include "lexer.h"
#include "parser.h"
#include <cstddef>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

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
                [this](FuncStmtNode& n) {
                    currentFunc = FunctionInfo{.name = n.name, .retType = n.retType, .id = functionCounter++};
                    for (ASTNode*& parameterNode : n.parameters) {
                        std::visit(overloads {
                            [this](VariableDeclareNode& n) {
                                currentFunc.paramType.emplace_back(n.type);
                            },
                            [this](auto& n) {print_error("only variable declaration can be done in the function declaration"); }
                        },*parameterNode);
                    }
                    functions.emplace(currentFunc.name, currentFunc);      
                    n.id = currentFunc.id;   
                },
                [this](auto& n) {}
            }, node);
        }

        for (ASTNode& node : nodes) {
            std::visit(overloads {
                [this](VariableDeclareNode& n) {
                    globalSymbolTable.emplace(n.name, SymboleInfo{.type = n.type, .stackOffset = 0, .name = n.name, .isGlobal = true, .size = typeSize[n.type.kind]});
                    if (n.info != nullptr) {
                        codeSemanticAnalyses(*n.info);
                        Type retType = type_precision(*n.info);
                        if (retType != n.type) {
                            Type promote = tryPromote(retType, n.type);
                            if (promote == retType) {
                                print_error("The variable is a "+n.type+" but tryed to declare it a "+promote);
                            }
                            n.info = arena.allocate<ASTNode>(PromotionNode{promote, retType, n.info});
                        }
                    }
                },
                [this](FuncStmtNode& n) {
                    currentFunc = functions[n.name];
                    scopeStack.push_back({});
                    for (ASTNode*& parameterNode : n.parameters) {
                        std::visit(overloads {
                            [this](VariableDeclareNode& n) {
                                scopeStack.back().emplace(n.name, SymboleInfo{n.type, 0});
                                if (n.info != nullptr) {
                                    codeSemanticAnalyses(*n.info);
                                    Type retType = type_precision(*n.info);
                                    if (retType != n.type) {
                                        Type promote = tryPromote(retType, n.type);
                                        if (promote == retType) {
                                            print_error("The variable is a "+n.type+" but tryed to declare it a "+promote);
                                        }
                                        n.info = arena.allocate<ASTNode>(PromotionNode{promote, retType, n.info});
                                    }
                                }
                            },
                            [this](auto& n) {print_error("only variable declaration can be done in the function declaration"); }
                        },*parameterNode);
                    }
                    if (n.code != nullptr)
                        codeSemanticAnalyses(*n.code);
                    bool hasreturn = hasReturn(*n.code);
                    if (n.retType != typeTable["void"] && !hasreturn) {
                        print_error("Function supposed to return a "+n.retType+" but doesn't return anything");
                    }
                    scopeStack.pop_back();
                    offset = 0;
                },
                [this](const EnumDeclNode& n) {
                    size_t max_value = 0;
                    for (const auto& member : n.members) {
                        if (static_cast<size_t>(member.second) > max_value) {
                            max_value = member.second;
                        }
                    }
                    globalEnumRegistry.emplace(n.name, EnumVariantInfo{.backing_type = max_value > 255 ? Type{Type::Kind::UINT16} : Type{Type::Kind::UINT8}});
                    for (const auto&[member,value] : n.members) {
                        globalEnumRegistry.emplace(n.name+"::"+member, EnumVariantInfo{value, max_value > 255 ? Type{Type::Kind::UINT16} : Type{Type::Kind::UINT8}});
                    }
                },
                [this](const StructDeclNode& n) {
                    StructTypeInfo info;
                    structSize(info);
                    globalStructRegistery.emplace(n.structName, info);
                },
                [this](const auto& n) {
                    print_error("only variables and function declare at file root"); // fall back incase the parser didn't already took care of it
                }
            }, node);
        }

        return {errorNumber, warningNumber};
    }

    void Semantic::structSize(StructTypeInfo& info) {
        size_t ret = 0;
        size_t maxAlignment = 1;
        for (auto& field : info.fields) {
            size_t size;
            size_t alignment;
            if (field.type != Type{Type::Kind::STRUCT}) {
                size = typeSize[field.type.kind];
                alignment = size;
            }
            else {
                const StructTypeInfo& infoOther = globalStructRegistery[field.type.name];
                size = infoOther.size;
                alignment = infoOther.alignment;
            }
            maxAlignment = std::max(maxAlignment, alignment);
            
            ret = (ret + alignment - 1) & ~(alignment - 1);
            field.offset = ret;
            ret += size;
        }

        ret = (ret + maxAlignment - 1) & ~(maxAlignment - 1);
        info.alignment = maxAlignment;
        info.size = ret;
    }
    
    bool Semantic::hasReturn(const ASTNode& n) {
        return std::visit(overloads{
            [this](const ReturnStmtNode& n) {
                return true;
            },
            [this](const BlockStmtNode& n) {
                for (const auto& node : n.codes) {
                    if (hasReturn(*node)) return true;
                }
                return false;
            },
            [this](const IfStmtNode& n) {
                if (hasReturn(*n.ifNode)) {
                    if (n.elseNode != nullptr && hasReturn(*n.elseNode)) return true;
                }
                return false;
            },
            [this](const auto& n) {
                return false;
            }
        },n);
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
                    PromotionNode node{promoted, retVal, n.expression};
                    n.expression = arena.allocate<ASTNode>(node);
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
                            n.right = arena.allocate<ASTNode>(PromotionNode{right,promote,n.right});
                        }
                        n.left = arena.allocate<ASTNode>(PromotionNode{promote,left,n.left});
                    }
                }
                else {
                    Type l = type_precision(*n.left);
                    Type r = type_precision(*n.right);
                    n.precision = resolve_type(l, r);
                }
            },
            [this](VariableDeclareNode& n) {
                if (scopeStack.back().contains(n.name)) {
                    print_error("Variable "+n.name+" was already declared somewhere else");
                }
                size_t size;
                size_t alignment;
                if (n.type.kind != Type::Kind::STRUCT) {
                    size = typeSize[n.type.kind];
                    alignment = size;
                }
                else {
                    size = globalStructRegistery[n.type.name].size;
                    alignment = globalStructRegistery[n.type.name].alignment;
                }
                offset = (offset + alignment - 1) & ~(alignment - 1);
                scopeStack.back().emplace(n.name, SymboleInfo{n.type, offset});
                offset += size;
                if (n.info != nullptr) {
                    codeSemanticAnalyses(*n.info);
                    Type retType = type_precision(*n.info);
                    if (retType != n.type) {
                        Type promote = tryPromote(retType, n.type);
                        if (promote == retType) {
                            print_error("The variable is a "+n.type+" but tried to declare it a "+promote);
                        }
                        n.info = arena.allocate<ASTNode>(PromotionNode{promote, retType, n.info});
                    }
                }
            },
            [this](VariableAccessNode& n) {
                char found = 0;
                for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                    if (it->contains(n.name) || globalSymbolTable.contains(n.name)) {
                        found = 1;
                        if (it->contains(n.name)) {
                            n.precision = it->at(n.name).type;
                            n.info = it->at(n.name);
                        }
                        else {
                            n.precision = globalSymbolTable[n.name].type;
                            n.info = globalSymbolTable[n.name];
                        }
                    }
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
                        n.operand = arena.allocate<ASTNode>(PromotionNode{promote, typePrecision, n.operand});
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
                    n.condition = arena.allocate<ASTNode>(PromotionNode{promote, condType, n.condition});
                }
                scopeStack.push_back({});
                codeSemanticAnalyses(*n.ifNode);
                scopeStack.pop_back();
                if (n.elseNode != nullptr) {
                    scopeStack.push_back({});
                    codeSemanticAnalyses(*n.elseNode);
                    scopeStack.pop_back();
                }
            },
            [this](BlockStmtNode& n) {
                scopeStack.push_back({});
                for (ASTNode*& blockNode : n.codes) {
                    codeSemanticAnalyses(*blockNode);
                }
                scopeStack.pop_back();
            },
            [this](FuncCallStmtNode& n) {
                if (!functions.contains(n.name)) {
                    print_error("Function "+n.name+" unknown");
                }
                FunctionInfo func = functions[n.name];
                if (n.callParameters.size() != func.paramType.size()) {
                    print_error("The function "+func.name+" only take "+std::to_string(func.paramType.size())+" arguments but "+std::to_string(n.callParameters.size())+" where given");
                }
                n.retType = func.retType;
                for (unsigned int i = 0; i < func.paramType.size() && i < n.callParameters.size(); ++i) {
                    const ASTNode* param = n.callParameters[i];
                    Type& type = func.paramType[i];
                    Type paramType = type_precision(*param);
                    if (type != paramType) {
                        Type promote = tryPromote(paramType, type);
                        if (promote == paramType) {
                            print_error("Expected argument type "+type+" but received a "+promote+" for parameter "+std::to_string(i));
                        }
                        n.callParameters[i] = arena.allocate<ASTNode>(PromotionNode{promote, paramType, n.callParameters[i]});
                    } 
                }
                n.id = func.id;
            },
            [this](VariableModNode& n) {
                codeSemanticAnalyses(*n.variable);
                codeSemanticAnalyses(*n.info);
                Type retType = type_precision(*n.info);
                SymboleInfo var = resolveVariable(*n.variable);
                if (retType != var.type) {
                    Type promote = tryPromote(retType, var.type);
                    if (promote == retType) {
                        print_error("The variable is a "+var.type+" but tryed to asigne a "+promote+" to it");
                    }
                    n.info = arena.allocate<ASTNode>(PromotionNode{promote, retType, n.info});
                }
                n.precision = var.type;
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
            [this](EnumAccessNode& n) {
                std::string name = n.enumName+"::"+n.enumMember;
                if (!globalEnumRegistry.contains(name)) {
                    print_error("unknown enum member "+name);
                }
            },
            [this](StructAccessNode& n) {
                codeSemanticAnalyses(*n.firstPart);
                Type base = type_precision(*n.firstPart);
                n.baseType = base;
                const StructTypeInfo& info = globalStructRegistery[base.name];
                auto it = std::find(
                    info.fields.begin(),
                    info.fields.end(),
                    StructField{n.fieldName}
                );
                if (it == info.fields.end()) {
                    print_error("Struct "+base.name+" doesn't have "+n.fieldName+" as a field");
                }
                n.info = *it;
                n.finalType = it->type;
            },
            [this](StructDeclNode& n) {
                print_error("Can't declare an enum inside a function"); // will maybe be changed later though probably not
            },
            [this](EnumDeclNode& n) {
                print_error("Can't declare an enum inside a function"); // will maybe be changed later though probably not
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

    SymboleInfo Semantic::resolveVariable(const ASTNode& n) {
        return std::visit(overloads {
            [&](const VariableAccessNode& n) {
                bool found = false;
                for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                    if (it->contains(n.name))
                        return it->at(n.name);
                if (globalSymbolTable.contains(n.name))
                    return globalSymbolTable[n.name];
                }
                return SymboleInfo{Type{Type::Kind::INT32}, 0};
            },
            [&](const StructAccessNode& n) {
                SymboleInfo base = resolveVariable(*n.firstPart);

                StructTypeInfo& info = globalStructRegistery.at(base.type.name);

                auto& field = *std::find(info.fields.begin(), info.fields.end(), StructField{n.fieldName});

                return SymboleInfo{
                    field.type,
                    0
                };
            },
            [&](const auto&) {
                print_error("Expression is not assignable");
                return SymboleInfo{Type{Type::Kind::INT32}, 0};
            }
        }, n);
    }

    Type Semantic::type_precision(const ASTNode& node) {
        return std::visit(overloads {
            [this](const Int32LiteralNode& n) {return Type{.kind = Type::Kind::INT32};},
            [this](const Float32LiteralNode& n) {return Type{.kind = Type::Kind::FLOAT32};},
            [this](const VariableAccessNode& n) {
                for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                    if (it->contains(n.name))
                        return it->at(n.name).type;
                }
                if (globalSymbolTable.contains(n.name))
                    return globalSymbolTable.at(n.name).type;
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
            [this](const EnumAccessNode& n) {
                return globalEnumRegistry[n.enumName+"::"+n.enumMember].backing_type;
            },
            [this](const StructAccessNode& n) {
                Type base = type_precision(*n.firstPart);
                StructTypeInfo info = globalStructRegistery[base.name];
                return std::find(info.fields.begin(), info.fields.end(), StructField{n.fieldName})->type;
            },
            [this](const auto&) {print_error("Uh?"); return Type{.kind = Type::Kind::INT32};}
        }, node);
    }

    Type Semantic::resolve_type(const Type left, const Type right) {
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

    Type Semantic::tryPromote(const Type currentType, const Type promoteTo) {
        if (resolve_type(currentType, promoteTo) == promoteTo)
            return promoteTo;
        int currentSize = typeSize[currentType.kind];
        int targetSize = typeSize[promoteTo.kind];
        if (currentType.kind == Type::Kind::ENUM) {
            currentSize = typeSize[globalEnumRegistry[currentType.name].backing_type.kind];
        }
        if (promoteTo.kind == Type::Kind::ENUM) {
            targetSize = typeSize[globalEnumRegistry[promoteTo.name].backing_type.kind];
        }
        if (targetSize > currentSize) 
            return promoteTo; 
        if (currentSize == targetSize) 
            return promoteTo;
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
