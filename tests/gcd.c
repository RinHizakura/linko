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

int main()
{
    uint64_t r = gcd64(18, 9);
    printf("%ld\n", r);
    return 0;
}
