#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stddef.h>

typedef struct elf elf_t;
typedef union elf_inner elf_inner_t;

struct elf {
    elf_inner_t *inner;
    size_t sz;
};

int elf_init(elf_t *elf, int fd);
int elf_check_valid(elf_t *elf);
void elf_close(elf_t *elf);

#endif
