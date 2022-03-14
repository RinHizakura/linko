#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

uint64_t gcd64(uint64_t u, uint64_t v)
{
    if (!u || !v)
        return u | v;
    while (v) {
        uint64_t t = v;
        v = u % v;
        u = t;
    }
    return u;
}

uint64_t gcdadd(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
    uint64_t x = gcd64(a, b);
    uint64_t y = gcd64(c, d);
    printf("%ld %ld\n", x, y);
    puts("hello!\n");

    return x + y;
}

int main()
{
    printf("%ld\n", gcdadd(210, 240, 180, 144));
    return 0;
}
