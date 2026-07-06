#include "ir.h"

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

            default:
                throw std::runtime_error("Unsupported type");
        }
    }
}