#include "ir.h"
#include <cstdint>
#include <string>
#include "irTranslator.h"

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

namespace nbuIR {
    IRTranslator::IRTranslator(const std::vector<nbuFrontend::ASTNode>& nodes) : nodes{nodes} {} 

    void IRTranslator::Translate() {
        for (const auto& n : nodes) {
            std::visit(overloads {
                [&](const nbuFrontend::FuncStmtNode& n) {

                },
                [&](const auto& n) {}
            },n);
        }
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::FuncStmtNode& n) {
        prog.functions.emplace_back();
        IRFunction& func = prog.functions.back();
        func.name = n.name;
        currentFunc = &func;
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::IfStmtNode& n) {
        IRBlock& ifBlock = currentFunc->block.emplace_back();
        IRBlock* elseBlock = nullptr;
        if (n.elseNode != nullptr) elseBlock = &currentFunc->block.emplace_back();
        IRBlock& endBlock = currentFunc->block.emplace_back();

        ifBlock.label = "IF_" + std::to_string(blockCounter);
        if (n.elseNode != nullptr) elseBlock->label = "ELSE_" + std::to_string(blockCounter);
        endBlock.label = "END_" + std::to_string(blockCounter);

        // will do the thingies later
        ++blockCounter;
    }

    void IRTranslator::TranslateStmt(const nbuFrontend::BlockStmtNode& n) {

    }

    void IRTranslator::TranslateStmt(const nbuFrontend::ReturnStmtNode& n) {

    }

    Val IRTranslator::TranslateExpr(const nbuFrontend::ASTNode& n) {
        return std::visit(overloads {
            [&](const nbuFrontend::Int32LiteralNode& n) {
                return Val(static_cast<int64_t>(n.value),nbuFrontend::Type{nbuFrontend::Type::Kind::INT32});
            },
            [&](const nbuFrontend::Float32LiteralNode& n) {
                return Val(n.value,nbuFrontend::Type{nbuFrontend::Type::Kind::FLOAT32});
            },
            [&](const nbuFrontend::BinaryOpNode& n) {
                Val lf = TranslateExpr(*n.left);
                Val rf = TranslateExpr(*n.right);

                Val tmp = makeTemp(n.precision);
                emitBinary(n.op, tmp, lf, rf);
                return tmp;
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                Val lf;
                if (n.info != nullptr)
                    lf = TranslateExpr(*n.info);
                Val dst(n.name, Val::Type::LOC, n.type);
                emitDeclaration(dst, lf);
                return dst;
            },
            [&](const nbuFrontend::VariableAccessNode& n) {
                return Val(n.name, Val::Type::LOC, n.precision); // will be changed since I currently don't know how to differenciate between glob and local
            },
            [&](const nbuFrontend::UnaryOpNode& n) {
                Val lf = TranslateExpr(*n.operand);

                Val tmp = makeTemp(n.precision);
                emitUnary(n.op, tmp, lf);
                return tmp;
            },
            [&](const nbuFrontend::FuncCallStmtNode& n) {
                std::vector<Val> params;
                if (n.callParameters.size() != 0) {
                    for (const auto& par : n.callParameters)
                        params.push_back(TranslateExpr(*par));
                }

                Val func = Val(n.name, Val::Type::LAB);
                Val ret = makeTemp(n.retType);
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
                Val tmp = makeTemp(n.topromote);
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
                Val tmp = makeTemp(t);
                emitRead(tmp, lf);
                return tmp;
            },
            [&](const nbuFrontend::writeAddrNode& n) {
                Val dst = TranslateExpr(*n.addr);
                Val lf = TranslateExpr(*n.value);
                emitWrite(dst, lf);
                return dst;
            },
            [&](const auto& n) {return makeTemp(nbuFrontend::Type{nbuFrontend::Type::Kind::INT32});}
        },n);
    }

    Val IRTranslator::makeTemp(const nbuFrontend::Type& t) {
        return Val("t"+std::to_string(tempCounter++),Val::Type::TEMP,t);
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
            // will add the others later
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
}