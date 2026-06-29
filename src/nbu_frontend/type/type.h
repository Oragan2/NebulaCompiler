#ifndef TYPE_H
#define TYPE_H

#include <cstddef>
#include <iostream>
#include <vector>

namespace nbuFrontend {
    struct Type {
        enum class Kind {
            FLOAT32, FLOAT64,
            INT32, INT64,
            UINT32, UINT64,
            VADDR, PADDR
        };
        Kind kind;
        std::string name;
        bool operator!=(const Type& other) const;
        bool operator==(const Type& other) const;
    };

    std::ostream& operator<<(std::ostream& os, Type token); 
    std::string operator+(const std::string& str, Type token);
    std::string type_to_str(Type token);

    class ArenaAllocator {
        public:

        ArenaAllocator(size_t size);
        
        template<typename T, typename... Args>
        T* allocate(Args&&... args) {
            size_t bytes = sizeof(T);

            bytes = (bytes + 7) & ~7;

            if (pointer + bytes > size) {
                throw std::runtime_error("Not enough memory for allocation");
            }
            T* ptr = reinterpret_cast<T*>(&page[pointer]);
            pointer += bytes;
            new (static_cast<void*>(ptr)) T{std::forward<Args>(args)...};
            return ptr;
        }

        ~ArenaAllocator();

        private:
        std::vector<std::byte> page;
        size_t pointer = 0;
        size_t size;
        inline static bool exist = false;
    };

    extern ArenaAllocator arena;
}

#endif