#ifndef CODEGEN_H
#define CODEGEN_H

#include "../../nbu_frontend/parser/parser.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace nbuBackend {
    enum class Register : uint8_t {
        A = 0x01, // Accumulator
        B = 0x02, // Base
        C = 0x03, // Counter
        D = 0x04, // Data
        XMM0 = 0x10,
        XMM1 = 0x11,
        RSP = 0x05,
        RBP = 0x06,
        None = 0x00
    };

    std::string regToStr(const Register&);
    std::string WordType(int bytesize);

    enum class Op {
        ADD, SUB, MUL, DIV, MOV, PUSH, POP, RET, SYSCALL,
        XOR, NOT, AND, OR, SHL, SAR, CBW, CWD, CDQ, CQO
    };

    std::string opToStr(const Op& op);

    struct MemoryOperand {
        Register baseReg = Register::RBP;
        int offset = 0;
        
        bool isIndexed = false;
        Register indexReg = Register::C;
        int scale = 1;

        std::string globalLabel;
        bool isGlobal = false;
    };

    struct Operand {
        enum class Type {None, Reg, Imm, Mem};
        Type type = Type::None;
        
        Register reg = Register::None;
        int64_t immValue = 0;
        MemoryOperand mem;

        Operand() : type(Type::None) {}
        Operand(Register r) : type{Type::Reg}, reg{r} {}
        Operand(int64_t imm) : type(Type::Imm), immValue(imm) {}
        Operand(int imm) : type(Type::Imm), immValue(imm) {}
        Operand(MemoryOperand m) : type(Type::Mem), mem(m) {}
    };

    std::string formatOperand(const Operand& op, int byteSize);
    
    class CodeGen {
        public:
        CodeGen(std::vector<nbuFrontend::ASTNode>& nodes, std::ofstream& file, std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums);
        void generate();

        private:
        CodeGen();
        int calcStructSize(nbuFrontend::StructTypeInfo& info);
        void nodeCalcString(const nbuFrontend::ASTNode& n);
        void calcOffsets(const nbuFrontend::ASTNode& n);
        void runOffsetWalker(const nbuFrontend::ASTNode& n);
        void structOffsets(const nbuFrontend::Type& n, std::string name);
        void fieldPrint(const nbuFrontend::StructTypeInfo& info, nbuFrontend::Type type, std::string name);
        std::string strDivision(nbuFrontend::Type type);
        std::string getFlatKey(const nbuFrontend::ASTNode& node);
        bool isConstant(const nbuFrontend::ASTNode& node);

        void emit(const Op& op, const Operand& dst=Operand(), const Operand& src=Operand(), int byteSize=4);
        void emitConv(const Register& dst, int dstSize, const Register& src, int srcSize, bool srcSigned);
        void emitLabel(const std::string& name);
        void emitComment(const std::string& comment);

        std::vector<nbuFrontend::ASTNode>& nodes;
        std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs;
        std::unordered_map<std::string, int> localOffsetMap;
        int localOffsetCursor;
        int alignOffset;
        std::string currentLabel;
        std::unordered_set<std::string> globalVariable;
        std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums;
        std::ofstream& file;
        std::stringstream data;
        std::stringstream bss;
        std::stringstream text;
        std::stringstream rodata;
        unsigned int labelCounter = 0;
    };
}

#endif