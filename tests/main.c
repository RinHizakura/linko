#include <stdio.h>
#include "linko.h"

typedef uint64_t (*gcd64_t)(uint64_t a, uint64_t b, uint64_t c, uint64_t d);

int main()
{
    linko_t l;

    int ret = linko_init(&l, "build/gcd", LINKO_EXEC);
    if (ret) {
        printf("linko init failed\n");
        return -1;
    }

    gcd64_t gcdadd = (gcd64_t) linko_find_symbol(&l, "gcdadd");
    if (gcdadd == NULL) {
        printf("linko find symbol failed\n");
        return -1;
    }

    printf("gcd %ld\n", gcdadd(16, 4, 12, 3));
    linko_close(&l);

    return 0;
}
