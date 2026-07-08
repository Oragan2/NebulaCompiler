#ifndef IRTRANSLATOR_H
#define IRTRANSLATOR_H

#include <vector>
#include "../../nbu_frontend/type/type.h"
#include "../../nbu_frontend/parser/parser.h"
#include "../ir/ir.h"

namespace nbuIR {
    class IRTranslator {
        public:
        IRTranslator(const std::vector<nbuFrontend::ASTNode>& nodes, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums);
        IRProgram Translate();

        private:
        void TranslateStmt(const nbuFrontend::ASTNode& n);
        void TranslateStmt(const nbuFrontend::FuncStmtNode& n);
        void TranslateStmt(const nbuFrontend::IfStmtNode& n);
        void TranslateStmt(const nbuFrontend::BlockStmtNode& n);
        void TranslateStmt(const nbuFrontend::ReturnStmtNode& n);
        void TranslateStmt(const nbuFrontend::writeAddrNode& n);
        void TranslateStmt(const nbuFrontend::VariableDeclareNode& n);

        Val TranslateExpr(const nbuFrontend::ASTNode& n);

        Val makeTemp(const Type& t);

        void emitBinary(nbuFrontend::TokenType op, const Val& dst, const Val& lf, const Val& rf);
        void emitLocalDeclaration(const Val& dst, const Val& lf);
        void emitGlobaleclaration(const Val& dst, const Val& lf=Val{});
        void emitUnary(nbuFrontend::TokenType op, const Val& dst, const Val& lf);
        void emitCall(const Val& func, const Val& ret, std::vector<Val> params);
        void emitAssign(const Val& var, const Val& lf);
        void emitConv(const Val& dst, const Val& lf);
        void emitRead(const Val& dst, const Val& lf);
        void emitWrite(const Val& dst, const Val& lf);
        void emitReturn(const Val& lf);
        void emitJMP(const Val& lab);
        void emitJE(const Val& lab);
        
        IRTranslator();
        const std::vector<nbuFrontend::ASTNode>& nodes;
        std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums;
        std::string getFlatKey(const nbuFrontend::ASTNode& n);
        IRProgram prog;
        IRFunction* currentFunc = nullptr;
        IRBlock* currentBlock = nullptr;
        unsigned int tempCounter = 0;
        unsigned int blockCounter = 0;
        unsigned int functionCounter = 0;
    };
}
#endif