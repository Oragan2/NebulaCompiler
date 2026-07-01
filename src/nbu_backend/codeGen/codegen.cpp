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
    
    void CodeGen::emit(const std::string& op, const std::string& dst, const std::string& src) {
        text << "\t" << op;
        if (!dst.empty())
            text << " "+dst;
        if (!src.empty())
            text << ", " << src;
        text << "\n";
    }

    void CodeGen::emitLabel(const std::string& name) {
        text << name << ":\n";
        currentLabel = name;
    }

    void CodeGen::emitComment(const std::string& comment) {
        text << "\t; " << comment << "\n";
    }

    void CodeGen::generate() {
        data << "section .data\n";
        bss << "section .bss\n";
        text << "section .text\n";
        rodata << "section .rodata\n";

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
                        emitLabel(n.name);
                    }
                    else {
                        text << "\tglobal _start" << "\n";
                        emitLabel("_start");
                    }
                    calcOffsets(*n.code);
                    emit("push","rbp");
                    emit("mov","rbp","rsp");
                    emit("sub","rsp",std::to_string(alignOffset));
                    nodeCalcString(*n.code);
                },
                [&](const auto&) {}
            },node);
        }
        file << data.rdbuf();
        file << bss.rdbuf();
        file << rodata.rdbuf();
        file << text.rdbuf();
    }

    std::string CodeGen::strWordType(nbuFrontend::Type type) {
        signed char size = nbuFrontend::typeSize[type.kind];
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

    void CodeGen::fieldPrint(const nbuFrontend::StructTypeInfo& info, nbuFrontend::Type type, std::string name) {
        for (const auto& [fieldName, field] : info.fields) {
            if (field.kind != nbuFrontend::Type::Kind::STRUCT) {
                int offset = localOffsetMap[name + fieldName];
                std::string modifier = strWordType(field);
                emit("mov", modifier + " [rbp" + std::to_string(offset) + "]", "0");
                emitComment(name+fieldName+" "+std::to_string(offset));
            }
            else {
                fieldPrint(info,field, name+fieldName+".");
            }
        }
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
                return "cwde";
        }
    }

    void CodeGen::nodeCalcString(const nbuFrontend::ASTNode& n) {
        std::visit(overloads {
            [&](const nbuFrontend::BlockStmtNode& n) {
                for (const auto& node : n.codes) {
                    nodeCalcString(*node);
                }
            },
            [&](const nbuFrontend::asmNode& n) {
                text << "\t"+n.rawAsm+"\n";
            },
            [&](const nbuFrontend::ReturnStmtNode& n) {
                if (n.expression != nullptr)
                    nodeCalcString(*n.expression);
                if (currentLabel != "_start") {
                    emit("mov","rsp","rbp");
                    emit("pop","rbp");
                    emit("ret");
                }
                else {
                    emit("mov","edi","eax");
                    emit("mov","eax","60");
                    emit("syscall");
                }
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                if (n.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                    if (n.info != nullptr && !isConstant(*n.info))
                        nodeCalcString(*n.info);
                    std::string dst = strWordType(n.type)+" [rbp"+std::to_string(localOffsetMap[n.name])+"]";
                    if (n.info == nullptr) {
                        std::string reg = strRegistery(n.type);
                        emit("xor", reg, reg);
                    }
                    if (isConstant(*n.info)) 
                        nodeCalcString(*n.info);

                    std::string src = strRegistery(n.type);
                    emit("mov",dst,src);
                    emitComment(n.name+" "+std::to_string(localOffsetMap[n.name]));
                }
                else {
                    nbuFrontend::StructTypeInfo info = structs[n.type.name];
                    fieldPrint(info, n.type, n.name+".");
                }
            }, 
            [&](const nbuFrontend::BinaryOpNode& n) {
                std::string registery = strRegistery(n.precision);
                std::string scratchReg = (registery == "rax") ? "rcx" : "ecx";
                std::string op = (registery == "xmm0") ? "movss" : "mov";
                if (registery == "xmm0") 
                    scratchReg = "xmm1";
                nodeCalcString(*n.left);
                if (registery == "xmm0") {
                    emit("sub","rsp","8");
                    emit("movss","[rsp]","xmm0");
                }
                else
                    emit("push","rax");
                nodeCalcString(*n.right);
                emit(op,scratchReg,registery);
                if (registery == "xmm0") {
                    emit("movss","xmm0","[rsp]");
                    emit("add","rsp","8");
                }
                else
                    emit("pop","rax");
                if (n.op != nbuFrontend::TokenType::SLASH && n.op != nbuFrontend::TokenType::PERCENT) {
                    std::string op = strOperand(n.op);
                    if (registery == "xmm0") op += "ss";
                    emit(op,registery,scratchReg);
                }
                else { 
                    emit(strDivision(n.precision));
                    emit("idiv",scratchReg);
                    if (n.op == nbuFrontend::TokenType::PERCENT) {
                        std::string remainderReg = (registery == "rax") ? "rdx" : "edx";
                        emit("mov", registery, remainderReg);
                    }
                }
            },
            [&](const nbuFrontend::VariableAccessNode& n) {
                std::string op;
                if (n.precision.kind != nbuFrontend::Type::Kind::FLOAT32 || n.precision.kind != nbuFrontend::Type::Kind::FLOAT64) op = "mov";
                else op = "movss";
                if (localOffsetMap.contains(n.name)) {
                    std::string reg = strRegistery(n.precision);
                    std::string src = "dword [rbp" + std::to_string(localOffsetMap[n.name]) + "]";
                    emit(op, reg, src);
                    emitComment(n.name+" "+std::to_string(localOffsetMap[n.name]));
                }
                else {
                    std::string reg = strRegistery(n.precision);
                    std::string src = "["+n.name+"]";
                    emit(op,reg,src);
                    emitComment(n.name);
                }
            },
            [&](const nbuFrontend::StructAccessNode& n) {
                std::string name = getFlatKey(*n.firstPart);
                std::string reg = strRegistery(structs[name].fields[n.fieldName]);
                std::string src = "dword [rbp" + std::to_string(localOffsetMap[name+"."+n.fieldName]) + "]";
                emit("mov",reg,src);
                emitComment(name+"."+n.fieldName);
            },
            [&](const nbuFrontend::Int32LiteralNode& n) {
                emit("mov","eax",std::to_string(n.value));
            },
            [&](const nbuFrontend::Float32LiteralNode& n) {
                std::string floatLabel = ".LC" + std::to_string(labelCounter++);
    
                rodata << "\t" << floatLabel << ": dd " << n.value << "\n";
    
                emit("movss", "xmm0", "[rel " + floatLabel + "]");
            },
            [&](const nbuFrontend::EnumAccessNode& n) {
                std::string reg = strRegistery(enums[n.enumName].backing_type);
                std::string src = std::to_string(enums[n.enumName+"::"+n.enumMember].raw_value);
                emit("mov", reg, src);
            },
            [&](const nbuFrontend::VariableModNode& n) {
                nodeCalcString(*n.info);
                std::string flatKey = getFlatKey(*n.variable);
                std::string src = strRegistery(n.precision);
                std::string dst;
                std::string op;
                if (n.precision.kind != nbuFrontend::Type::Kind::FLOAT32 && n.precision.kind != nbuFrontend::Type::Kind::FLOAT64) op = "mov";
                else op = "movss";
                std::cout << n.precision << " " << flatKey << " " << op << std::endl;
                if (localOffsetMap.find(flatKey) != localOffsetMap.end()) {
                    dst = strWordType(n.precision)+" [rbp" + std::to_string(localOffsetMap[flatKey]) + "]";
                } else {
                    dst = strWordType(n.precision)+" [" + flatKey + "]";
                }
                emit(op, dst, src);
                emitComment(flatKey+" "+std::to_string(localOffsetMap[flatKey]));
            },
            [&](const nbuFrontend::PromotionNode& n) {
                nodeCalcString(*n.info);

                std::string srcType = nbuFrontend::type_to_str(n.was);
                std::string dstType = nbuFrontend::type_to_str(n.topromote);

                bool srcSigned = (srcType[0] != 'u');
                bool srcFloat = (n.was.kind == nbuFrontend::Type::Kind::FLOAT32 || n.was.kind == nbuFrontend::Type::Kind::FLOAT64);
                bool dstFloat = (n.topromote.kind == nbuFrontend::Type::Kind::FLOAT32 || n.topromote.kind == nbuFrontend::Type::Kind::FLOAT64);
                int srcSize = nbuFrontend::typeSize[n.was.kind];
                int dstSize = nbuFrontend::typeSize[n.topromote.kind];

                if (srcSize == dstSize) return;

                if (srcSize < dstSize && !srcFloat && !dstFloat) {
                    if (srcSigned) {
                        if (srcSize == 1 && dstSize == 4) emit("movsx", "eax", "al");
                        if (srcSize == 1 && dstSize == 2) emit("movsx", "ax", "al");
                        if (srcSize == 2 && dstSize == 4) emit("movsx", "eax", "ax");
                        if (srcSize == 4 && dstSize == 8) emit("movsxd", "rax", "eax");
                    } else {
                        if (srcSize == 1 && dstSize == 4) emit("movzx", "eax", "al");
                        if (srcSize == 1 && dstSize == 2) emit("movzx", "ax", "al");
                        if (srcSize == 2 && dstSize == 4) emit("movzx", "eax", "ax");
                        if (srcSize == 4 && dstSize == 8) emit("mov", "eax", "eax");
                    }
                }
                if (dstFloat && !srcFloat) 
                    emit("cvtsi2ss", "xmm0","eax");
                if (srcFloat && !dstFloat)
                    emit("cvttss2si", "eax", "xmm0");
            },
            [&](const auto&) {
                text << "\n";
            }
        },n);
    }

    std::string CodeGen::getFlatKey(const nbuFrontend::ASTNode& node) {
        return std::visit(overloads {
        [&](const nbuFrontend::VariableAccessNode& var) {
            return var.name;
        },
        [&](const nbuFrontend::StructAccessNode& sac) {
            return getFlatKey(*sac.firstPart) + "." + sac.fieldName;
        },
        [&](const auto&) {
            return std::string("");
        }
    }, node);
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
        if (type.kind != nbuFrontend::Type::Kind::FLOAT32 && type.kind != nbuFrontend::Type::Kind::FLOAT64)
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
        else {
            return "xmm0";
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
                    int fieldSize = nbuFrontend::typeSize[enums[node.type.name].backing_type.kind];
                
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
                int fieldSize = nbuFrontend::typeSize[field.kind];

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