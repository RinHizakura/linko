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
                           Elf64_Shdr *output_sec_header);
int elf_lookup_rela(elf_t *elf,
                    char *symbol,
                    unsigned char type_info,
                    Elf64_Rela *rela);
int elf_lookup_symbol(elf_t *elf,
                      char *symbol,
                      unsigned char type_info,
                      Elf64_Sym *sym);
int elf_lookup_function(elf_t *elf, char *symbol, Elf64_Sym *sym);
void *elf_copy_section(elf_t *elf, Elf64_Shdr *sec_header, uint8_t *output);
void elf_close(elf_t *elf);

#endif
