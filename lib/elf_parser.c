#include "elf_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

union elf_inner {
    Elf64_Ehdr *header;
    uint8_t *data;
};

static inline Elf64_Shdr *get_section_header(uint8_t *elf_file,
                                             Elf64_Ehdr *elf_header,
                                             int offset)
{
    return (Elf64_Shdr *) (elf_file + elf_header->e_shoff +
                           elf_header->e_shentsize * offset);
}

int elf_init(elf_t *elf, int fd)
{
    struct stat st;

    if (fstat(fd, &st))
        return -1;

    elf->inner = malloc(sizeof(elf_t));
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

int elf_lookup_section_hdr(elf_t *elf,
                           char *name,
                           Elf64_Word sh_type,
                           Elf64_Shdr *output_sec_header)
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
            memcpy(output_sec_header,
                   get_section_header(elf_data, elf_header, i),
                   sizeof(Elf64_Shdr));
            return 0;
        }
    }

    return -1;
}

int elf_lookup_rela(elf_t *elf,
                    char *symbol,
                    unsigned char type_info,
                    Elf64_Rela *rela)
{
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;

    Elf64_Shdr rela_sec_header;
    int ret =
        elf_lookup_section_hdr(elf, ".rela.plt", SHT_RELA, &rela_sec_header);
    if (ret)
        return ret;
    Elf64_Rela *relatab = (Elf64_Rela *) (elf_data + rela_sec_header.sh_offset);

    Elf64_Shdr *symtab_sec_header =
        get_section_header(elf_data, elf_header, rela_sec_header.sh_link);
    Elf64_Sym *symtab = (Elf64_Sym *) (elf_data + symtab_sec_header->sh_offset);

    Elf64_Shdr dynstr_sec_header;
    ret =
        elf_lookup_section_hdr(elf, ".dynstr", SHT_STRTAB, &dynstr_sec_header);
    if (ret)
        return ret;
    char *strtab = (char *) elf_data + dynstr_sec_header.sh_offset;

    unsigned int rela_cnt = rela_sec_header.sh_size / sizeof(Elf64_Rela);

    for (unsigned int i = 0; i < rela_cnt; i++) {
        if (ELF64_R_TYPE(relatab[i].r_info) == type_info) {
            const char *f_symbol =
                strtab + symtab[ELF64_R_SYM(relatab[i].r_info)].st_name;
            if (!strcmp(symbol, f_symbol)) {
                memcpy(rela, &relatab[i], sizeof(Elf64_Rela));
                return 0;
            }
        }
    }
    return -1;
}

int elf_lookup_symbol(elf_t *elf,
                      char *symbol,
                      unsigned char type_info,
                      Elf64_Sym *sym)
{
    Elf64_Shdr symtab_sec_header;
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;
    int ret =
        elf_lookup_section_hdr(elf, ".symtab", SHT_SYMTAB, &symtab_sec_header);
    if (ret)
        return ret;

    Elf64_Sym *symtab = (Elf64_Sym *) (elf_data + symtab_sec_header.sh_offset);

    Elf64_Shdr *strtab_sec_header =
        get_section_header(elf_data, elf_header, symtab_sec_header.sh_link);
    char *strtab = (char *) elf_data + strtab_sec_header->sh_offset;

    unsigned int symbol_cnt = symtab_sec_header.sh_size / sizeof(Elf64_Sym);
    for (unsigned int i = 0; i < symbol_cnt; i++) {
        if (ELF64_ST_TYPE(symtab[i].st_info) == type_info) {
            const char *f_symbol = strtab + symtab[i].st_name;

            if (!strcmp(symbol, f_symbol)) {
                memcpy(sym, &symtab[i], sizeof(Elf64_Sym));
                return 0;
            }
        }
    }
    return -1;
}

int elf_lookup_function(elf_t *elf, char *symbol, Elf64_Sym *sym)
{
    return elf_lookup_symbol(elf, symbol, STT_FUNC, sym);
}

void *elf_copy_section(elf_t *elf, Elf64_Shdr *sec_header, uint8_t *output)
{
    uint8_t *elf_data = elf->inner->data;
    return memcpy(output, elf_data + sec_header->sh_offset,
                  sec_header->sh_size);
}

void elf_close(elf_t *elf)
{
    munmap(elf->inner->data, elf->sz);
    free(elf->inner);
}
