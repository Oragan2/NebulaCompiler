#include "codegen.h"
#include "../../nbu_frontend/parser/parser.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;


namespace nbuBackend {
    CodeGen::CodeGen(std::vector<nbuFrontend::ASTNode>& nodes, std::ofstream& file, std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums) : nodes{nodes}, file{file}, structs{structs}, enums{enums} {}
    void CodeGen::generate() {
        std::stringstream data;
        std::stringstream bss;
        std::stringstream text;

        data << "section .data\n";
        bss << "section .bss\n";
        text << "section .text\n";

        for (const auto& node : nodes) {
            std::visit(overloads {
                [&](const nbuFrontend::VariableDeclareNode& n) {
                    if (n.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                        data << "\t" + n.name + ": d";
                        switch(nbuFrontend::typeSize[n.type.kind]) {
                            case 1:
                                data << "b ";
                                break;
                            case 2:
                                data << "w ";
                                break;
                            case 4:
                                data << "d ";
                                break;
                            case 8:
                                data << "q ";
                                break;
                        }
                        if (n.info == nullptr)
                            data << "0\n";
                        else {
                            data << std::visit(overloads {
                                [](const nbuFrontend::Int32LiteralNode& n) {
                                    return std::to_string(n.value);
                                },
                                [](const nbuFrontend::Float32LiteralNode& n) {
                                    return std::to_string(n.value);
                                },
                                [](const auto&) {
                                    return std::string{"0"};
                                }
                            }, *n.info);
                            data << "\n";
                        }
                    }
                    else {
                        bss << "\t" << n.name << ": resb " << calcStructSize(structs[n.type.name]) << "\n";
                    }
                    globalVariable.emplace(n.name);
                },
                [&](const nbuFrontend::FuncStmtNode& n) {
                    if (n.name != "main") {
                        text << "\tglobal " << n.name << "\n";
                        text << n.name << ":\n";
                    }
                    else {
                        text << "\tglobal _start" << "\n";
                        text << "_start:\n";
                    }
                    calcOffsets(*n.code);
                    text << "\tpush rbp\n";
                    text << "\tmov rbp, rsp\n";
                    text << "\tsub rsp, "+std::to_string(alignOffset)+"\n";
                    text << nodeCalcString(*n.code);
                    text << "\n";
                },
                [&](const auto&) {}
            },node);
        }
        file << data.rdbuf();
        file << bss.rdbuf();
        file << text.rdbuf();
    }

    std::string CodeGen::strWordType(nbuFrontend::Type type) {
        char size = nbuFrontend::typeSize[type.kind];
        if (type.kind == nbuFrontend::Type::Kind::ENUM) {
            size = nbuFrontend::typeSize[enums[type.name].backing_type.kind];
        }
        switch (size) {
            case 1:
                return "byte";
            case 2:
                return "word";
            case 4:
                return "dword";
            case 8:
                return "qword";
            default:
                return "byte";
        }
    }

    std::string CodeGen::fieldPrint(const nbuFrontend::StructTypeInfo& info, nbuFrontend::Type type, std::string name) {
        std::string ret ;
        for (const auto& [fieldName, field] : info.fields) {
            if (field.kind != nbuFrontend::Type::Kind::STRUCT) {
                int offset = localOffsetMap[name + fieldName];
                std::string modifier = strWordType(field);
                ret += "\tmov " + modifier + " [rbp " + std::to_string(offset) + "], 0\n";
            }
            else {
                ret += fieldPrint(info,field, name+fieldName+".");
            }
        }
        return ret;
    }

    std::string strOperand(nbuFrontend::TokenType op) {
        switch(op) {
            case nbuFrontend::TokenType::AND:
                return "and";
            case nbuFrontend::TokenType::OR:
                return "or";
            case nbuFrontend::TokenType::NOT:
                return "not";
            case nbuFrontend::TokenType::SLASH:
                return "idiv";
            case nbuFrontend::TokenType::PLUS:
                return "add";
            case nbuFrontend::TokenType::MINUS:
                return "sub";
            case nbuFrontend::TokenType::STAR:
                return "imul";
            case nbuFrontend::TokenType::SHIFTL:
                return "shl";
            case nbuFrontend::TokenType::SHIFTR:
                return "sar";
            case nbuFrontend::TokenType::EXCLAMATION:
                return "not"; // for now
            default:
                return "issue";
        }
    }

    std::string CodeGen::strDivision(nbuFrontend::Type type) {
char size = nbuFrontend::typeSize[type.kind];
        if (type.kind == nbuFrontend::Type::Kind::ENUM) {
            size = nbuFrontend::typeSize[enums[type.name].backing_type.kind];
        }
        switch (size) {
            case 1:
            case 2:
                return "cwde";
            case 4:
                return "cdq";
            case 8:
                return "cqo";
            default:
                return "byte";
        }
    }

    std::string CodeGen::nodeCalcString(const nbuFrontend::ASTNode& n) {
        std::string ret;
        std::visit(overloads {
            [&](const nbuFrontend::BlockStmtNode& n) {
                for (const auto& node : n.codes) {
                    ret += nodeCalcString(*node);
                }
            },
            [&](const nbuFrontend::asmNode& n) {
                ret += "\t"+n.rawAsm+"\n";
            },
            [&](const nbuFrontend::ReturnStmtNode& n) {
                ret += nodeCalcString(*n.expression);
                ret += "\tret\n";
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                if (n.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                    if (n.info != nullptr && !isConstant(*n.info))
                        ret += nodeCalcString(*n.info);
                    ret += "\tmov ";
                    ret += strWordType(n.type);
                    ret += " [rbp"+std::to_string(localOffsetMap[n.name])+"], ";
                    if (n.info == nullptr)
                        ret += std::string("0");
                    if (isConstant(*n.info)) 
                        ret += nodeCalcString(*n.info);
                    else 
                        ret += strRegistery(n.type);
                    ret += "\n";
                }
                else {
                    nbuFrontend::StructTypeInfo info = structs[n.type.name];
                    ret += fieldPrint(info, n.type, n.name+".");
                }
            }, 
            [&](const nbuFrontend::BinaryOpNode& n) {
                std::string registery = strRegistery(n.precision);
                std::string scratchReg = (registery == "rax") ? "rcx" : "ecx";
                if (isConstant(*n.left))
                    ret += "\tmov "+registery+", ";
                ret += nodeCalcString(*n.left);
                ret += "\n\tpush rax\n";
                if (isConstant(*n.right))
                    ret += "\tmov "+registery+", ";
                ret += nodeCalcString(*n.right);
                ret += "\n\tmov " + scratchReg + ", " + registery + "\n";
                ret += "\n\tpop rax\n";
                if (n.op != nbuFrontend::TokenType::SLASH && n.op != nbuFrontend::TokenType::PERCENT) {
                    ret += "\t"+strOperand(n.op) + " "+registery+", "+scratchReg+"\n";
                }
                else { 
                    ret += "\t"+strDivision(n.precision);
                    ret += "\n\tidiv "+scratchReg;
                }
                ret += "\n";
            },
            [&](const nbuFrontend::VariableAccessNode& n) {
                ret += "[rbp"+std::to_string(localOffsetMap[n.name])+"]";
            },
            [&](const nbuFrontend::Int32LiteralNode& n) {
                ret += std::to_string(n.value);
            },
            [&](const nbuFrontend::Float32LiteralNode& n) {
                ret += std::to_string(n.value);
            },
            [&](const nbuFrontend::EnumAccessNode& n) {
                ret += std::to_string(enums[n.enumName+"::"+n.enumMember].raw_value);
            },
            [&](const auto&) {
                ret += "\n";
            }
        },n);
        return ret;
    }

    bool CodeGen::isConstant(const nbuFrontend::ASTNode& node) {
        return std::visit(overloads {
            [](const nbuFrontend::Int32LiteralNode&) {return true;},
            [](const nbuFrontend::Float32LiteralNode&) {return true;},
            [](const nbuFrontend::VariableAccessNode&) {return true;},
            [](const nbuFrontend::EnumAccessNode&) {return true;},
            [](const auto&) {return false;}
        }, node);
    }

    std::string CodeGen::strRegistery(nbuFrontend::Type type) {
        char size = nbuFrontend::typeSize[type.kind];
        if (type.kind == nbuFrontend::Type::Kind::ENUM) {
            size = nbuFrontend::typeSize[enums[type.name].backing_type.kind];
        }
        switch (size) {
            case 1:
                return "al";
            case 2:
                return "ax";
            case 4:
                return "eax";
            case 8:
                return "rax";
            default:
                return "al";
        }
    }

    void CodeGen::calcOffsets(const nbuFrontend::ASTNode& n) {
        localOffsetCursor = 0;
        localOffsetMap.clear();
        
        runOffsetWalker(n);

        alignOffset = (localOffsetCursor + 15) & ~15;
    }

    void CodeGen::runOffsetWalker(const nbuFrontend::ASTNode& n) {
        std::visit(overloads {
            [&](const nbuFrontend::VariableDeclareNode& node) {
                if (node.type.kind == nbuFrontend::Type::Kind::ENUM) {
                    int fieldSize = nbuFrontend::typeSize[enums[node.name].backing_type.kind];
                
                    if (localOffsetCursor % fieldSize != 0) {
                        localOffsetCursor += (fieldSize - (localOffsetCursor % fieldSize));
                    }

                    localOffsetCursor += fieldSize;
                    localOffsetMap.emplace(node.name, -localOffsetCursor);
                }
                else if (node.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                    int fieldSize = nbuFrontend::typeSize[node.type.kind];
                
                    if (localOffsetCursor % fieldSize != 0) {
                        localOffsetCursor += (fieldSize - (localOffsetCursor % fieldSize));
                    }

                    localOffsetCursor += fieldSize;
                    localOffsetMap.emplace(node.name, -localOffsetCursor);
                }
                else {
                    structOffsets(node.type,node.name+".");
                }
            },
            [&](const nbuFrontend::BlockStmtNode& node) {
                for (const auto& code : node.codes) {
                    runOffsetWalker(*code);
                }
            },
            [&](const auto&) {}
        },n);
    }

    void CodeGen::structOffsets(const nbuFrontend::Type& n, std::string name) {
        nbuFrontend::StructTypeInfo info = structs[n.name];
        for (const auto& [fieldName, field] : info.fields) {
            if (n.kind == nbuFrontend::Type::Kind::ENUM) {
                    int fieldSize = nbuFrontend::typeSize[enums[n.name].backing_type.kind];
                
                    if (localOffsetCursor % fieldSize != 0) {
                        localOffsetCursor += (fieldSize - (localOffsetCursor % fieldSize));
                    }

                    localOffsetCursor += fieldSize;
                    localOffsetMap.emplace(n.name, -localOffsetCursor);
                }
            else if (field.kind != nbuFrontend::Type::Kind::STRUCT) {
                int fieldSize = nbuFrontend::typeSize[field.kind]; // 🌟 Fixed: use field.kind, not n.kind!

                // Handle struct internal alignment
                if (localOffsetCursor % fieldSize != 0) {
                    localOffsetCursor += (fieldSize - (localOffsetCursor % fieldSize));
                }

                localOffsetCursor += fieldSize;
                localOffsetMap.emplace(name + fieldName, -localOffsetCursor);
            }
            else 
                structOffsets(field, name+fieldName+".");
        }
    }

    int CodeGen::calcStructSize(nbuFrontend::StructTypeInfo& info) {
        int currentSize = 0;
        int maxAlignment = 1; // Track the largest alignment requirements in the struct
        for (const auto& [name, type] : info.fields) {
            int fieldSize = 0;
            int fieldAlignment = 1;

            if (type.kind != nbuFrontend::Type::Kind::STRUCT) {
                fieldSize = nbuFrontend::typeSize[type.kind];
                fieldAlignment = fieldSize;
            } else {
                fieldSize = calcStructSize(structs[type.name]);
                fieldAlignment = 8;
            }

            if (fieldAlignment > maxAlignment) {
                maxAlignment = fieldAlignment;
            }

            if (currentSize % fieldAlignment != 0) {
                currentSize += (fieldAlignment - (currentSize % fieldAlignment));
            }

            // Place the field
            currentSize += fieldSize;
        }
        if (currentSize % maxAlignment != 0) {
            currentSize += (maxAlignment - (currentSize % maxAlignment));
        }
        return currentSize;
    }
}