#include "elf_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

int elf_init(elf_t *elf, int fd)
{
    struct stat st;

    if (fstat(fd, &st))
        return -1;

    elf->inner = malloc(sizeof(elf_inner_t));
    if (elf == NULL)
        return -1;

    elf->sz = st.st_size;
    elf->inner->data = mmap(0, elf->sz, PROT_READ, MAP_PRIVATE, fd, 0);
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

int elf_lookup_shdr(elf_t *elf,
                    char *name,
                    Elf64_Word sh_type,
                    Elf64_Shdr **output_sec_header)
{
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;
    Elf64_Shdr *shstrtab_sec_header =
        get_section_header(elf_data, elf_header, elf_header->e_shstrndx);
    char *shstrtab = (char *) (elf_data + shstrtab_sec_header->sh_offset);

    for (Elf64_Half i = 0; i < elf_header->e_shnum; i++) {
        Elf64_Shdr *sec_header = get_section_header(elf_data, elf_header, i);
        char *sec_name = shstrtab + sec_header->sh_name;

        /* For a request that implies (possibly) valid section type identifier,
         * we add a fast path here to quickly skip the non-expected header.
         * Otherwise, the name of section should be compare directly. */
        if (sh_type < SHT_LOUSER && sh_type != sec_header->sh_type)
            continue;

        if (!strcmp(name, sec_name)) {
            *output_sec_header = get_section_header(elf_data, elf_header, i);
            return 0;
        }
    }

    return -1;
}

int elf_lookup_symbol(elf_t *elf,
                      char *symbol,
                      unsigned char type_info,
                      Elf64_Sym **sym)
{
    Elf64_Shdr *symtab_sec_header;
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;
    int ret = elf_lookup_shdr(elf, ".symtab", SHT_SYMTAB, &symtab_sec_header);
    if (ret)
        return ret;

    Elf64_Sym *symtab = (Elf64_Sym *) (elf_data + symtab_sec_header->sh_offset);

    Elf64_Shdr *strtab_sec_header =
        get_section_header(elf_data, elf_header, symtab_sec_header->sh_link);
    char *strtab = (char *) elf_data + strtab_sec_header->sh_offset;

    unsigned int symbol_cnt = symtab_sec_header->sh_size / sizeof(Elf64_Sym);
    for (unsigned int i = 0; i < symbol_cnt; i++) {
        if (ELF64_ST_TYPE(symtab[i].st_info) == type_info) {
            const char *f_symbol = strtab + symtab[i].st_name;

            if (!strcmp(symbol, f_symbol)) {
                *sym = &symtab[i];
                return 0;
            }
        }
    }
    return -1;
}

int elf_lookup_function(elf_t *elf, char *symbol, Elf64_Sym **sym)
{
    return elf_lookup_symbol(elf, symbol, STT_FUNC, sym);
}

void elf_close(elf_t *elf)
{
    munmap(elf->inner->data, elf->sz);
    free(elf->inner);
}
