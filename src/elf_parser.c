#include "elf_parser.h"
#include <elf.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

union elf_inner {
    Elf64_Ehdr *header;
    uint8_t *data;
};

int elf_init(elf_t *elf, int fd)
{
    struct stat st;

    if (fstat(fd, &st))
        return -1;

    elf->inner = malloc(sizeof(elf_t));
    if (elf == NULL)
        return -1;

    elf->sz = st.st_size;
    elf->inner->data =
        mmap(0, elf->sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (elf->inner->data == MAP_FAILED)
        return -1;

    return 0;
}

int elf_check_valid(elf_t *elf)
{
    Elf64_Ehdr *elf_header = elf->inner->header;
    unsigned char *ident = elf_header->e_ident;

    if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' ||
        ident[3] != 'F')
        return -1;

    return 0;
}

void elf_close(elf_t *elf)
{
    free(elf->inner);
}
