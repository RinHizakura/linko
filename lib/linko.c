#include "linko.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "elf_parser.h"
#include "utils/align.h"

/* This is a set of x86 instructions which can hang the execution */
static uint8_t hang[10] = {0xf3, 0x0f, 0x1e, 0xfa, 0x55,
                           0x48, 0x89, 0xe5, 0xeb, 0xfe};

static void hang_func()
{
    printf("Hi, you are hanged by me!\n");
    while (1)
        ;
}

static int linko_create_map(linko_t *l)
{
    Elf64_Shdr *text_sec_header;
    int ret = elf_lookup_section_hdr(&l->elf, ".text", SHT_PROGBITS,
                                     &text_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *plt_sec_header;
    ret = elf_lookup_section_hdr(&l->elf, ".plt.sec", SHT_PROGBITS,
                                 &plt_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *got_sec_header;
    ret =
        elf_lookup_section_hdr(&l->elf, ".got", SHT_PROGBITS, &got_sec_header);
    if (ret)
        return LINKO_ERR;

    size_t reserved_sz = (got_sec_header->sh_addr - text_sec_header->sh_addr);
    size_t sz = plt_sec_header->sh_size + reserved_sz + got_sec_header->sh_size;
    l->map_sz = ALIGN_UP(sz, sysconf(_SC_PAGESIZE));
    l->map_region = mmap(NULL, l->map_sz, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (l->map_region == MAP_FAILED)
        return LINKO_ERR;

    l->plt_region = l->map_region;
    l->text_region = l->map_region + plt_sec_header->sh_size;
    l->got_region = l->text_region + reserved_sz;

    memcpy(l->text_region, l->elf.inner->data + text_sec_header->sh_offset,
           text_sec_header->sh_size);
    memcpy(l->plt_region, l->elf.inner->data + plt_sec_header->sh_offset,
           plt_sec_header->sh_size);
    memcpy(l->got_region, l->elf.inner->data + got_sec_header->sh_offset,
           got_sec_header->sh_size);
    // memcpy(l->got_region, hang, 10);

    return LINKO_NO_ERR;
}

static int linko_protect_map(linko_t *l)
{
    if (mprotect(l->map_region, l->map_sz, PROT_READ | PROT_EXEC))
        return LINKO_ERR;
    return LINKO_NO_ERR;
}

static int linko_do_rela(linko_t *l)
{
    uint8_t *elf_data = l->elf.inner->data;
    Elf64_Ehdr *elf_header = l->elf.inner->header;

    Elf64_Shdr *got_sec_header;
    int ret =
        elf_lookup_section_hdr(&l->elf, ".got", SHT_PROGBITS, &got_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *rela_sec_header;
    ret = elf_lookup_section_hdr(&l->elf, ".rela.plt", SHT_RELA,
                                 &rela_sec_header);
    if (ret)
        return LINKO_ERR;
    Elf64_Rela *relatab =
        (Elf64_Rela *) (elf_data + rela_sec_header->sh_offset);

    Elf64_Shdr *symtab_sec_header =
        get_section_header(elf_data, elf_header, rela_sec_header->sh_link);
    Elf64_Sym *symtab = (Elf64_Sym *) (elf_data + symtab_sec_header->sh_offset);

    Elf64_Shdr *dynstr_sec_header =
        get_section_header(elf_data, elf_header, symtab_sec_header->sh_link);
    char *strtab = (char *) elf_data + dynstr_sec_header->sh_offset;

    unsigned int rela_cnt = rela_sec_header->sh_size / sizeof(Elf64_Rela);
    for (unsigned int i = 0; i < rela_cnt; i++) {
        if (ELF64_R_TYPE(relatab[i].r_info) == R_X86_64_JUMP_SLOT) {
            const char *f_symbol =
                strtab + symtab[ELF64_R_SYM(relatab[i].r_info)].st_name;
            printf("rela symbol %s, ", f_symbol);
            printf("off %lx\n", relatab[i].r_offset);
            printf("off %lx\n", (uint64_t) hang_func);
            *(uint64_t *) (l->got_region +
                           (relatab[i].r_offset - got_sec_header->sh_addr)) =
                (uint64_t) hang_func;
        }
    }

    return LINKO_NO_ERR;
}

int linko_init(linko_t *l, char *obj_file, TYPE file_type)
{
    int fd = open(obj_file, O_RDONLY);
    if (fd < 0)
        return LINKO_ERR;

    l->type = file_type;
    int ret = elf_init(&l->elf, fd);
    close(fd);
    if (ret)
        return LINKO_ERR;

    if (elf_check_valid(&l->elf))
        return LINKO_ERR;

    if (linko_create_map(l))
        return LINKO_ERR;

    if (linko_do_rela(l))
        return LINKO_ERR;

    if (linko_protect_map(l))
        return LINKO_ERR;

    return LINKO_NO_ERR;
}

void *linko_find_symbol(linko_t *l, char *symbol)
{
    /* FIXME: we can cache the founded header instead of searching it
     * again. */
    Elf64_Shdr *text_sec_header;
    int ret = elf_lookup_section_hdr(&l->elf, ".text", SHT_PROGBITS,
                                     &text_sec_header);
    if (ret)
        return NULL;

    Elf64_Sym *sym;
    if (elf_lookup_function(&l->elf, symbol, &sym))
        return NULL;

    return (void *) (l->text_region +
                     (sym->st_value - text_sec_header->sh_addr));
}

void linko_close(linko_t *l)
{
    munmap(l->map_region, l->map_sz);
    elf_close(&l->elf);
}
