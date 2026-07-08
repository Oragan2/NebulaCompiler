#include "ir.h"
#include <string>

namespace nbuIR {
    Type toIRType(const nbuFrontend::Type& t)
    {
        switch (t.kind)
        {
            case nbuFrontend::Type::Kind::INT8:    return Type::I8;
            case nbuFrontend::Type::Kind::INT16:   return Type::I16;
            case nbuFrontend::Type::Kind::INT32:   return Type::I32;
            case nbuFrontend::Type::Kind::INT64:   return Type::I64;

            case nbuFrontend::Type::Kind::UINT8:   return Type::U8;
            case nbuFrontend::Type::Kind::UINT16:  return Type::U16;
            case nbuFrontend::Type::Kind::UINT32:  return Type::U32;
            case nbuFrontend::Type::Kind::UINT64:  return Type::U64;

            case nbuFrontend::Type::Kind::FLOAT32: return Type::F32;
            case nbuFrontend::Type::Kind::FLOAT64: return Type::F64;

            case nbuFrontend::Type::Kind::VADDR:   return Type::U64;
            case nbuFrontend::Type::Kind::PADDR:   return Type::U64;

            case nbuFrontend::Type::Kind::VOID:    return Type::V;

            case nbuFrontend::Type::Kind::STRUCT: return Type::U64;

            default: 
                throw std::runtime_error("Unsupported type "+nbuFrontend::type_to_str(t));
        }
    }

    std::string irProgramToStr(const IRProgram &prog) {
        std::string ret;
        for (const auto& func : prog.functions) 
            ret += irFunctionToStr(func)+"\n";
        return ret;
    }

    std::string irFunctionToStr(const IRFunction &func) {
        std::string ret;
        ret += "f"+std::to_string(func.id)+"\n";
        for (const auto& block : func.blocks)
            ret += irBlockToStr(block)+"\n";
        return ret;
    }

    std::string irBlockToStr(const IRBlock &block) {
        std::string ret;
        ret += block.label+":\n";
        for (const auto& inst : block.instructions)
            ret += "\t"+irInstToStr(inst)+"\n";
        return ret;
    }

    std::string irInstToStr(const IRInst &instruct) {
        std::string ret;
        ret += opToStr(instruct.op);
        ret += ", "+valToStr(instruct.dst);
        ret += ", "+valToStr(instruct.lf);
        ret += ", "+valToStr(instruct.rf);
        for (int i = 0; instruct.args.size() > i; ++i) {
            ret += " "+valToStr(instruct.args[i]);
        }
        return ret;
    }

    std::string opToStr(const Op &op) {
        switch (op) {
            case Op::ADD:
                return "add";
            case Op::SUB:
                return "sub";
            case Op::MUL:
                return "mul";
            case Op::DIV:
                return "div";
            case Op::AND:
                return "and";
            case Op::OR:
                return "or";
            case Op::XOR:
                return "xor";
            case Op::NOT:
                return "not";
            case Op::MOV:
                return "mov";
            case Op::LOAD:
                return "load";
            case Op::STORE:
                return "store";
            case Op::CMP:
                return "cmp";
            case Op::JMP:
                return "jmp";
            case Op::JZ:
                return "jz";
            case Op::JNZ:
                return "jnz";
            case Op::CALL:
                return "call";
            case Op::RET:
                return "ret";
            case Op::CAST:
                return "cast";
            default:
                return "you forgot";
        }
    }

    std::string valToStr(const Val &val) {
        std::string ret;
        switch (val.type) {
            case Val::Type::NONE:
                return "";
            case Val::Type::LOC:
                ret += "LOC["+std::to_string(val.offset)+"]";
                ret += ":" + typeToStr(val.valueType);
                ret += " ";
                if (val.valueType != Type::F32 || val.valueType != Type::F64)
                    ret += std::to_string(val.i);
                else
                    ret += std::to_string(val.f);
                return ret;
            case Val::Type::GLO:
                ret += "GLO["+val.name+"]";
                ret += ":" + typeToStr(val.valueType);
                ret += " ";
                if (val.valueType != Type::F32 || val.valueType != Type::F64)
                    ret += std::to_string(val.i);
                else
                    ret += std::to_string(val.f);
                return ret;
            case Val::Type::CONST:
                ret += typeToStr(val.valueType);
                ret += " CONST(";
                if (val.valueType != Type::F32 || val.valueType != Type::F64)
                    ret += std::to_string(val.i);
                else
                    ret += std::to_string(val.f);
                return ret+")";
            case Val::Type::TEMP:
                ret += "t"+std::to_string(val.id);
                ret += ":"+typeToStr(val.valueType);
                ret += " ";
                if (val.valueType != Type::F32 || val.valueType != Type::F64)
                    ret += std::to_string(val.i);
                else
                    ret += std::to_string(val.f);
                return ret;
            case Val::Type::LABF:
                return "f"+std::to_string(val.id);
            case Val::Type::LABB:
                return val.name;
            default:
                return "You forgot";
        }
    }

    std::string typeToStr(const Type &type) {
        switch (type) {
            case Type::I8:
                return "i8";
            case Type::I16:
                return "i16";
            case Type::I32:
                return "i32";
            case Type::I64:
                return "i64";
            case Type::U8:
                return "u8";
            case Type::U16:
                return "u16";
            case Type::U32:
                return "u32";
            case Type::U64:
                return "u64";
            case Type::F32:
                return "f32";
            case Type::F64:
                return "f64";
            case Type::V:
                return "void";
            default:
                return "you forgot";
        }
    }
}