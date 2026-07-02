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
    
    void CodeGen::emit(const Op& op, const Operand& dst, const Operand& src, int byteSize) {
        std::string reg1;
        switch (dst.type) {
            case Operand::Type::None:
                break;
            case Operand::Type::Imm:
                reg1 = std::to_string(dst.immValue);
                break;
            case Operand::Type::Reg:
                
                break;
            case Operand::Type::Mem:
                reg1 += WordType(byteSize) + " ";
                if (dst.mem.isGlobal) {
                    reg1 += "["+dst.mem.globalLabel+"]";
                }
                else {
                    reg1 += "["+regToStr(dst.mem.baseReg)+std::to_string(dst.mem.offset)+"]";
                }
        }
    }

    std::string regToStr(Register a) {
        switch (a) {
            case Register::A:
                return "a";
            case Register::B:
                return "b";
            case Register::C:
                return "c";
            case Register::D:
                return "d";
            case Register::XMM0:
                return "xmm0";
            case Register::XMM1:
                return "xmm1";
            case Register::RBP:
                return "rbp";
            case Register::RSP:
                return "rsp";
            case Register::None:
                return "none";
        }
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
                    emit(Op::PUSH,Register::RBP);
                    emit(Op::MOV,Register::RBP,Register::RSP);
                    emit(Op::SUB,Register::RSP,alignOffset);
                    nodeCalcString(*n.code);
                },
                [&](const auto&) {}
            },node);
        }
        file << "default rel\n";
        file << data.rdbuf();
        file << bss.rdbuf();
        file << rodata.rdbuf();
        file << text.rdbuf();
    }

    std::string CodeGen::WordType(int byteSize) {
        switch (byteSize) {
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
                MemoryOperand mem;
                mem.globalLabel = name+fieldName;
                mem.offset = offset;
                emit(Op::MOV, mem, 0);
                emitComment(name+fieldName+" "+std::to_string(offset));
            }
            else {
                fieldPrint(info,field, name+fieldName+".");
            }
        }
    }

    Op Operand(nbuFrontend::TokenType op) {
        switch(op) {
            case nbuFrontend::TokenType::AND:
                return Op::AND;
            case nbuFrontend::TokenType::OR:
                return Op::OR;
            case nbuFrontend::TokenType::NOT:
                return Op::NOT;
            case nbuFrontend::TokenType::SLASH:
                return Op::DIV;
            case nbuFrontend::TokenType::PLUS:
                return Op::ADD;
            case nbuFrontend::TokenType::MINUS:
                return Op::SUB;
            case nbuFrontend::TokenType::STAR:
                return Op::MUL;
            case nbuFrontend::TokenType::SHIFTL:
                return Op::SHL;
            case nbuFrontend::TokenType::SHIFTR:
                return Op::SAR;
            case nbuFrontend::TokenType::EXCLAMATION:
                return Op::NOT; // for now
            default:
                return Op::MOV;
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
                    emit(Op::MOV,Register::RSP,Register::RBP);
                    emit(Op::POP,Register::RBP);
                    emit(Op::RET);
                }
                else {
                    emit(Op::MOV,Register::D,Register::A);
                    emit(Op::MOV,Register::A,60);
                    emit(Op::SYSCALL);
                }
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                if (n.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                    if (n.info != nullptr && !isConstant(*n.info))
                        nodeCalcString(*n.info);
                    MemoryOperand mem;
                    mem.offset = localOffsetMap[n.name];
                    mem.scale = nbuFrontend::typeSize[n.type.kind];
                    mem.baseReg = Register::RSP;
                    if (n.info == nullptr) {
                        emit(Op::XOR, Register::A, Register::A);
                    }
                    if (isConstant(*n.info)) 
                        nodeCalcString(*n.info);
                    emit(Op::MOV,mem,Register::A);
                    emitComment(n.name+" "+std::to_string(localOffsetMap[n.name]));
                }
                else {
                    nbuFrontend::StructTypeInfo info = structs[n.type.name];
                    fieldPrint(info, n.type, n.name+".");
                }
            }, 
            [&](const nbuFrontend::BinaryOpNode& n) {
                bool isFloat = (n.precision.kind == nbuFrontend::Type::Kind::FLOAT32 || n.precision.kind == nbuFrontend::Type::Kind::FLOAT64);
                Register registery = isFloat ? Register::XMM0 : Register::A;
                Register scratch = isFloat ? Register::XMM1 : Register::C;
                nodeCalcString(*n.left);
                if (isFloat) {
                    emit(Op::SUB,Register::RSP,8);
                    MemoryOperand mem;
                    mem.offset = 0;
                    mem.baseReg = Register::RSP;
                    int size = nbuFrontend::typeSize[n.precision.kind];
                    emit(Op::MOV,mem,Register::XMM0,size);
                }
                else
                    emit(Op::PUSH,Register::A);
                nodeCalcString(*n.right);
                emit(Op::MOV,scratch,registery);
                if (isFloat) {
                    MemoryOperand mem;
                    mem.offset = 0;
                    mem.baseReg = Register::RSP;
                    emit(Op::MOV,Register::XMM0,mem);
                    emit(Op::ADD,Register::RSP,8);
                }
                else {
                    emit(Op::POP,Register::A);
                }
                if (n.op != nbuFrontend::TokenType::SLASH && n.op != nbuFrontend::TokenType::PERCENT) {
                    Op op = Operand(n.op);
                    emit(op,registery,scratch);
                }
                else { 
                    Op extensionOp;
                    switch (nbuFrontend::typeSize[n.precision.kind]) {
                        case 1:  extensionOp = Op::CBW; break; // AL -> AX
                        case 2:  extensionOp = Op::CWD; break; // AX -> DX:AX
                        case 4:  extensionOp = Op::CDQ; break; // EAX -> EDX:EAX
                        case 8:  extensionOp = Op::CQO; break; // RAX -> RDX:RAX
                        default: extensionOp = Op::CDQ; break;
                    }
                    emit(extensionOp);
                    emit(Op::DIV,scratch);
                    if (n.op == nbuFrontend::TokenType::PERCENT) {
                        emit(Op::MOV, registery, Register::D);
                    }
                }
            },
            [&](const nbuFrontend::VariableAccessNode& n) {
                Register reg = (n.precision.kind == nbuFrontend::Type::Kind::FLOAT32 || n.precision.kind == nbuFrontend::Type::Kind::FLOAT64) ? Register::XMM0 : Register::A;
                int size = nbuFrontend::typeSize[n.precision.kind];
                if (localOffsetMap.contains(n.name)) {
                    MemoryOperand mem;
                    mem.offset = localOffsetMap[mem.globalLabel];
                    emit(Op::MOV, reg, mem,size);
                    emitComment(n.name+" "+std::to_string(localOffsetMap[n.name]));
                }
                else {
                    MemoryOperand mem;
                    mem.globalLabel = n.name;
                    mem.isGlobal = true;
                    emit(Op::MOV,reg,mem,size);
                    emitComment(n.name);
                }
            },
            [&](const nbuFrontend::StructAccessNode& n) {
                std::string name = getFlatKey(*n.firstPart);
                Register reg = (structs[name].fields[n.fieldName] == nbuFrontend::Type{nbuFrontend::Type::Kind::FLOAT32} || structs[name].fields[n.fieldName] == nbuFrontend::Type{nbuFrontend::Type::Kind::FLOAT32}) ? Register::XMM0 : Register::A;
                std::string src = "dword [rbp" + std::to_string(localOffsetMap[name+"."+n.fieldName]) + "]";
                MemoryOperand mem;
                mem.baseReg = Register::RSP;
                mem.globalLabel = name+"."+n.fieldName;
                mem.offset = localOffsetMap[name+"."+n.fieldName];
                emit(Op::MOV,reg,mem);
                emitComment(name+"."+n.fieldName);
            },
            [&](const nbuFrontend::Int32LiteralNode& n) {
                emit(Op::MOV,Register::A,n.value);
            },
            [&](const nbuFrontend::Float32LiteralNode& n) {
                std::string floatLabel = "_flt_const_" + std::to_string(labelCounter++);
    
                rodata << "\t" << floatLabel << ": dd " << n.value << "\n";
                MemoryOperand mem;
                mem.globalLabel = floatLabel;
                mem.isGlobal = true;
                emit(Op::MOV, Register::XMM0,mem,4);
            },
            [&](const nbuFrontend::EnumAccessNode& n) {
                int src = enums[n.enumName+"::"+n.enumMember].raw_value;
                emit(Op::MOV, Register::A, src,nbuFrontend::typeSize[enums[n.enumName].backing_type.kind]);
            },
            [&](const nbuFrontend::VariableModNode& n) {
                bool isFloat = (n.precision.kind == nbuFrontend::Type::Kind::FLOAT32 || n.precision.kind == nbuFrontend::Type::Kind::FLOAT64);
                nodeCalcString(*n.info);
                std::string flatKey = getFlatKey(*n.variable);
                Register src = isFloat ? Register::XMM0 : Register::A;
                MemoryOperand mem;
                if (localOffsetMap.find(flatKey) != localOffsetMap.end()) {
                    mem.offset = localOffsetMap[flatKey];
                } else {
                    mem.isGlobal = true;
                    mem.globalLabel = flatKey;
                }
                emit(Op::MOV, mem, src);
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
                Register dst = dstFloat ? Register::XMM0 : Register::A;
                Register src = srcFloat ? Register::XMM0 : Register::A;

                emitConv(dst, dstSize, src, srcSize, srcSigned);
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