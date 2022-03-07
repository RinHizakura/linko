#include <stdio.h>
#include "linko.h"

#define ARGS_CNT 3

typedef uint64_t (*gcd64_t)(uint64_t a, uint64_t b);

int main(int argc, char *argv[])
{
    if (argc != ARGS_CNT) {
        printf(
            "The number of input arguments should be %d, but the input is %d\n",
            ARGS_CNT, argc);
        return -1;
    }

    linko_t l;

    int ret = linko_init(&l, argv[1]);
    if (ret) {
        printf("linko init failed\n");
        return -1;
    }

    gcd64_t gcd64 = (gcd64_t) linko_find_symbol(&l, argv[2]);
    if (gcd64 == NULL) {
        printf("linko find symbol failed\n");
        return -1;
    }

    printf("gcd %ld\n", gcd64(16, 4));
    linko_close(&l);

    return 0;
}
