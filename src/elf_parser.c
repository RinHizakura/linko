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

int elf_lookup_section_hdr(elf_t *elf,
                           char *name,
                           Elf64_Word sh_type,
                           Elf64_Half *idx)
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

        if (sec_header->sh_type == SHT_SYMTAB ||
            sec_header->sh_type == SHT_DYNSYM)

            if (!strcmp(name, sec_name)) {
                *idx = (uint16_t) i;
                return 0;
            }
    }

    return -1;
}

int elf_get_symbol(elf_t *elf, char *symbol)
{
    uint16_t idx;
    int ret = elf_lookup_section_hdr(elf, ".symtab", SHT_SYMTAB, &idx);
    if (ret)
        return ret;

    printf("idx of symtab: %d\n", idx);

    return 0;
}

void elf_close(elf_t *elf)
{
    free(elf->inner);
}
