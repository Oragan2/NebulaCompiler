#include "ir.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include "irTranslator.h"

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

namespace nbuIR {
    IRTranslator::IRTranslator(const std::vector<nbuFrontend::ASTNode>& nodes, std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums) : nodes{nodes}, structs{structs}, enums{enums} {} 

    void IRTranslator::Translate() {
        for (const auto& n : nodes) {
            TranslateStmt(n);
        }
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::ASTNode& n) {
        std::visit(overloads {
            [&](const nbuFrontend::FuncStmtNode& node) {TranslateStmt(node);},
            [&](const nbuFrontend::BlockStmtNode& node) {TranslateStmt(node);},
            [&](const nbuFrontend::ReturnStmtNode& node) {TranslateStmt(node);},
            [&](const nbuFrontend::IfStmtNode& node) {TranslateStmt(node);},
            [&](const nbuFrontend::VariableDeclareNode& node) {
            },
            [&](const auto& n) {}
        }, n);
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::FuncStmtNode& n) {
        prog.functions.emplace_back();
        IRFunction& func = prog.functions.back();
        func.id = n.id;
        currentFunc = &func;
        func.blocks.emplace_back();
        func.blocks.back().label = "entry";
        TranslateStmt(*n.code);
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::IfStmtNode& n) {
        IRBlock& ifBlock = currentFunc->blocks.emplace_back();
        TranslateExpr(*n.condition);
        ifBlock.label = "IF_"+std::to_string(blockCounter);
        currentBlock = &ifBlock;
        TranslateStmt(*n.ifNode);
        if (n.elseNode != nullptr) {
            IRBlock& elseBlock = currentFunc->blocks.emplace_back();
            currentBlock = &elseBlock;
            elseBlock.label = "ELSE_"+std::to_string(blockCounter);
            TranslateStmt(*n.elseNode);
        }
        IRBlock& endBlock = currentFunc->blocks.emplace_back();
        endBlock.label = "ENDIF_"+std::to_string(blockCounter);
        currentBlock = &endBlock;
        ++blockCounter;
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::BlockStmtNode& n) {
        for (const auto& node : n.codes) {
            TranslateStmt(*node);
        }
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::ReturnStmtNode& n) {

    }

    void IRTranslator::TranslateStmt(const nbuFrontend::writeAddrNode& n) {
        Val dst = TranslateExpr(*n.addr);
        Val lf = TranslateExpr(*n.value);
        emitWrite(dst, lf);
    }

    Val IRTranslator::TranslateExpr(const nbuFrontend::ASTNode& n) {
        return std::visit(overloads {
            [&](const nbuFrontend::Int32LiteralNode& n) {
                return Val(static_cast<int64_t>(n.value),Type::I32);
            },
            [&](const nbuFrontend::Float32LiteralNode& n) {
                return Val(n.value,Type::F32);
            },
            [&](const nbuFrontend::BinaryOpNode& n) {
                Val lf = TranslateExpr(*n.left);
                Val rf = TranslateExpr(*n.right);

                Val tmp = makeTemp(toIRType(n.precision));
                emitBinary(n.op, tmp, lf, rf);
                return tmp;
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                Val dst(n.vInfo.stackOffset, Val::Type::LOC, toIRType(n.type));
                if (n.info != nullptr) {
                    Val lf = TranslateExpr(*n.info);
                    emitDeclaration(dst, lf);
                }
                return dst;
            },
            [&](const nbuFrontend::VariableAccessNode& n) {
                return Val(n.info.stackOffset, Val::Type::LOC, toIRType(n.precision));
            },
            [&](const nbuFrontend::UnaryOpNode& n) {
                Val lf = TranslateExpr(*n.operand);

                Val tmp = makeTemp(toIRType(n.precision));
                emitUnary(n.op, tmp, lf);
                return tmp;
            },
            [&](const nbuFrontend::FuncCallStmtNode& n) {
                std::vector<Val> params;
                if (n.callParameters.size() != 0) {
                    for (const auto& par : n.callParameters)
                        params.push_back(TranslateExpr(*par));
                }

                Val func = makeLable(n.id);
                Val ret = makeTemp(toIRType(n.retType));
                emitCall(func, ret, std::move(params));
                return ret;
            },
            [&](const nbuFrontend::VariableModNode& n) {
                Val var = TranslateExpr(*n.variable);
                Val lf = TranslateExpr(*n.info);

                emitAssign(var, lf);
                return var;
            },
            [&](const nbuFrontend::PromotionNode& n) {
                Val lf = TranslateExpr(*n.info); // will always have a type
                Val tmp = makeTemp(toIRType(n.topromote));
                emitConv(tmp, lf);
                return tmp;
            },
            [&](const nbuFrontend::readAddrNode& n) {
                Val lf = TranslateExpr(*n.addr);
                nbuFrontend::Type t;
                switch (n.quantity) {
                    case 1:
                        t = nbuFrontend::Type{nbuFrontend::Type::Kind::UINT8};
                        break;
                    case 2:
                        t = nbuFrontend::Type{nbuFrontend::Type::Kind::UINT16};
                        break;
                    case 4:
                        t = nbuFrontend::Type{nbuFrontend::Type::Kind::UINT32};
                        break;
                    case 8:
                        t = nbuFrontend::Type{nbuFrontend::Type::Kind::UINT64};
                        break;
                }
                Val tmp = makeTemp(toIRType(t));
                emitRead(tmp, lf);
                return tmp;
            },
            [&](const nbuFrontend::asmNode& n) { return makeTemp(Type::U32);}, // will find implementation later
            [&](const nbuFrontend::EnumAccessNode& n) {
                Val val(static_cast<int64_t>(enums[n.enumName+"::"+n.enumMember].raw_value), toIRType(enums[n.enumName+"::"+n.enumMember].backing_type));
                return val;
            },
            [&](const nbuFrontend::StructAccessNode& n) {
                Val base = TranslateExpr(*n.firstPart);

                Val addr = makeTemp(Type::U64);
                emitBinary(
                    nbuFrontend::TokenType::PLUS,
                    addr,
                    base,
                    Val(static_cast<int64_t>(n.info.offset), Type::I32)
                );

                Val result = makeTemp(toIRType(n.finalType));
                emitRead(result, addr);
                return result;
            },
            [&](const auto& n) {return makeTemp(Type::I32);}
        },n);
    }

    Val IRTranslator::makeTemp(const Type& t) {
        Val ret;
        ret.id = tempCounter++;
        ret.type = Val::Type::TEMP;
        ret.valueType = t;
        return ret;
    }

    Val makeLable(size_t id) {
        Val ret;
        ret.id = id;
        ret.type = Val::Type::LAB;
        return ret;
    }

    void IRTranslator::emitBinary(nbuFrontend::TokenType op, const Val& dst, const Val& lf, const Val& rf) {
        Op instOp;
        switch (op) {
            case nbuFrontend::TokenType::PLUS:
                instOp = Op::ADD;
                break;
            case nbuFrontend::TokenType::MINUS:
                instOp = Op::SUB;
                break;
            case nbuFrontend::TokenType::PERCENT:
            case nbuFrontend::TokenType::SLASH:
                instOp = Op::DIV;
                break;
            case nbuFrontend::TokenType::STAR:
                instOp = Op::MUL;
                break;
            case nbuFrontend::TokenType::AND:
                instOp = Op::AND;
                break;
            case nbuFrontend::TokenType::OR:
                instOp = Op::OR;
                break;
            case nbuFrontend::TokenType::XOR:
                instOp = Op::XOR;
                break;
            case nbuFrontend::TokenType::NOT:
                instOp = Op::NOT;
                break;
            default:
                instOp = Op::ADD;
        }

        currentBlock->instructions.push_back(IRInst{instOp, dst, lf, rf});
    }

    void IRTranslator::emitUnary(nbuFrontend::TokenType op, const Val& dst, const Val& lf) {
        Op instOp;
        switch (op) {
            case nbuFrontend::TokenType::NOT:
            case nbuFrontend::TokenType::DIFFERENT:
                instOp = Op::NOT;
            case nbuFrontend::TokenType::MINUS:
                instOp = Op::SUB; // work by subbstracting the value * 2
            default: // The only three for now are handled aboves
                instOp = Op::NOT;
        }
        currentBlock->instructions.push_back(IRInst{instOp, dst, lf});
    }

    void IRTranslator::emitDeclaration(const Val& dst, const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::STORE, lf});
    }

    void IRTranslator::emitCall(const Val& func, const Val& ret, std::vector<Val> params) {
        currentBlock->instructions.push_back(IRInst{.op = Op::CALL, .dst = ret, .lf = func, .args = std::move(params)}); // dst is for the return value, lf is the function to call
    }

    void IRTranslator::emitAssign(const Val& var, const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::MOV, var, lf});
    }

    void IRTranslator::emitConv(const Val& dst, const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::CAST, dst, lf});
    }

    void IRTranslator::emitRead(const Val& dst, const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::LOAD, dst, lf});
    }
    
    void IRTranslator::emitWrite(const Val& dst, const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::STORE,dst, lf});
    }

    void IRTranslator::emitReturn(const Val& lf) {
        currentBlock->instructions.push_back(IRInst{Op::RET,Val{}, lf}); // tmp
    }
}