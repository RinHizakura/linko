#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stddef.h>

typedef struct elf elf_t;
typedef union elf_inner elf_inner_t;

struct elf {
    elf_inner_t *inner;
    size_t sz;
};

int elf_init(elf_t *elf, int fd);
int elf_check_valid(elf_t *elf);
int elf_lookup_section_hdr(elf_t *elf,
                           char *name,
                           Elf64_Word sh_type,
                           Elf64_Half *idx);
int elf_get_symbol(elf_t *elf, char *symbol, Elf32_Addr *addr);
void elf_close(elf_t *elf);

#endif
