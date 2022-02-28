#include <stdint.h>

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
