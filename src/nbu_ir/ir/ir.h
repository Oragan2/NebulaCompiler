#ifndef IR_H
#define IR_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "../../nbu_frontend/type/type.h"

namespace nbuIR {
    enum class Op {
        ADD,
        SUB,
        MUL,
        DIV,

        AND,
        OR,
        XOR,
        NOT,

        MOV,

        LOAD,
        STORE,

        CMP,

        JMP,
        JZ,
        JNZ,

        CALL,
        RET,

        CAST
    };

    enum class Type {
        I8,
        I16,
        I32,
        I64,        
        U8,
        U16,
        U32,
        U64,
        F32,
        F64,
        V
    };

    struct Val {
        enum class Type {
            NONE,
            LOC, GLO, CONST, TEMP,
            LABF, LABB
        };
        nbuIR::Type valueType = nbuIR::Type::V;
        Type type = Type::NONE;
        std::string name;
        int64_t i = 0;
        double f = 0;
        size_t offset = 0;
        size_t id = 0;
        Val(int64_t n, nbuIR::Type t) : type{Type::CONST}, i{n}, valueType{t} {}
        Val(double n, nbuIR::Type t) : type{Type::CONST}, f{n}, valueType{t} {}
        Val(size_t offset, Type type, nbuIR::Type t={}) : type{type}, valueType{t}, offset{offset} {}
        Val(std::string name, Type type, nbuIR::Type t={}) : type{type}, valueType{t}, name{name} {}
        Val() {}
    };

    Val makeFLable(size_t id);
    Val makeBLable(const std::string& name);

    struct IRInst {
        Op op;
        Val dst;
        Val lf;
        Val rf;

        std::vector<Val> args;
    };

    struct IRBlock {
        std::string label;
        std::vector<IRInst> instructions;
    };

    struct IRFunction {
        size_t id;
        Type retType;
        std::vector<IRBlock> blocks;
    };

    struct IRProgram {
        std::vector<IRInst> globals;
        std::vector<IRFunction> functions;
    };

    Type toIRType(const nbuFrontend::Type& t);
    std::string irInstToStr(const IRInst& instruct);
    std::string irBlockToStr(const IRBlock& block);
    std::string irFunctionToStr(const IRFunction& func);
    std::string irProgramToStr(const IRProgram& prog);
    std::string opToStr(const Op& op);
    std::string valToStr(const Val& val);
    std::string typeToStr(const Type& type);
}
#endif