#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stddef.h>

typedef struct elf elf_t;
typedef union elf_inner elf_inner_t;

union elf_inner {
    Elf64_Ehdr *header;
    uint8_t *data;
};

struct elf {
    elf_inner_t *inner;
    size_t sz;
};

static inline Elf64_Shdr *get_section_header(uint8_t *elf_file,
                                             Elf64_Ehdr *elf_header,
                                             int offset)
{
    return (Elf64_Shdr *) (elf_file + elf_header->e_shoff +
                           elf_header->e_shentsize * offset);
}

int elf_init(elf_t *elf, int fd);
int elf_check_valid(elf_t *elf);
int elf_lookup_shdr(elf_t *elf,
                    char *name,
                    Elf64_Word sh_type,
                    Elf64_Shdr **output_sec_header);
int elf_lookup_rela(elf_t *elf,
                    char *symbol,
                    unsigned char type_info,
                    Elf64_Rela **rela);
int elf_lookup_symbol(elf_t *elf,
                      char *symbol,
                      unsigned char type_info,
                      Elf64_Sym **sym);
int elf_lookup_function(elf_t *elf, char *symbol, Elf64_Sym **sym);
void elf_close(elf_t *elf);

#endif
