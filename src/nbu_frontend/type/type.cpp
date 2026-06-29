#include "type.h"
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace nbuFrontend {
    ArenaAllocator arena(4*1024*1204);

    bool Type::operator!=(const Type& other) const {
        return kind != other.kind;
    }
    
    bool Type::operator==(const Type& other) const {
        return kind == other.kind;
    }

    std::string type_to_str(Type type) {
        switch(type.kind) {
            case Type::Kind::INT32:
                return "int32";
            case Type::Kind::UINT32:
                return "uint32";
            case Type::Kind::INT64:
                return "int64";
            case Type::Kind::UINT64:
                return "uint64";
            case Type::Kind::FLOAT32:
                return "float32";
            case Type::Kind::FLOAT64:
                return "float64";
            case Type::Kind::VADDR:
                return "vaddr";
            case Type::Kind::PADDR:
                return "paddr";
            default:
                return "Unknown Type";
        }
    }

    std::string operator+(const std::string& str, Type type) {
        return str+type_to_str(type);
    }

    std::ostream& operator<<(std::ostream& os, Type type) {
        os << type_to_str(type);
        return os;
    }

    ArenaAllocator::ArenaAllocator(size_t size) {
        if (exist) throw std::runtime_error("Error, can't have more then one arena allocator exist at the same time");
        page = std::vector<std::byte>(size);
        this->size = size;
        exist = true;
    }

    ArenaAllocator::~ArenaAllocator() {
        exist = false;
    }
}