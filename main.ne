float32 cart = 12.2;

uint32 test(uint32 i)
    return i+1;

int64 test2(uint32 i, int64 v) {
    return i+v;
}

enum Colors {
    Red,
    Green,
    Blue
}

enum PageFlags {
    PRESENT = 1,
    WRITABLE = 2,
    USER = 4
}

struct TaskState {
    uint32 back_link,
    vaddr  esp0,
    uint32 ss0,
}

int32 main() {
    PageFlags a = PageFlags::PRESENT;
    TaskState b;
    b.back_link = 12;
    int32 x = 2*2/3;
    int32 y = x+1;
    int32 z = ~y;
    x = test(x);
    if (x == y)
        int32 tmp = 1;
    else if (x == z) {
        int32 tmp = 1;
        tmp = tmp+1;
    }
    tmp = 4;
    vaddr data = 123;
    write32(data,32);
    x = read32(data);
    cart = 13.1;
    asm {
        xor rax, rax
    }
    return z+y;
}

