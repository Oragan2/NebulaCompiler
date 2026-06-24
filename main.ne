float32 cart = 12.2;

uint32 test(int64 i)
    return i+1;

int32 main() {
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
    return x+y;
}
