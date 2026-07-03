#ifndef IR_H
#define IR_H

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

    struct Val {
        enum class Type {
            NONE,
            LOC, GLO, CONST, TEMP,
            LAB
        };
        nbuFrontend::Type valueType;
        Type type = Type::NONE;
        int64_t i = 0;
        float f = 0;
        std::string name;

        Val(int64_t n, nbuFrontend::Type t) : type{Type::CONST}, i{n}, valueType{t} {}
        Val(float n, nbuFrontend::Type t) : type{Type::CONST}, f{n}, valueType{t} {}
        Val(const std::string& v, Type type, nbuFrontend::Type t={}) : type{type}, name{v}, valueType{t} {}
        Val() {}
    };

    Val makeTemp();

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
        std::string name;
        std::vector<IRBlock> block;
    };

    struct IRProgram {
        std::vector<IRFunction> functions;
    };
}
#endif