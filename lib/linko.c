#include "linko.h"
#include <dlfcn.h>
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

static void *linko_copy_section(linko_t *l, uint8_t *dst, Elf64_Shdr *shdr)
{
    uint8_t *elf_data = l->elf.inner->data;
    return memcpy(dst, elf_data + shdr->sh_offset, shdr->sh_size);
}

static int linko_create_map(linko_t *l)
{
    Elf64_Shdr *text_sec_header;
    int ret = elf_lookup_shdr(&l->elf, ".text", SHT_PROGBITS, &text_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *plt_sec_header;
    ret = elf_lookup_shdr(&l->elf, ".plt.sec", SHT_PROGBITS, &plt_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *got_sec_header;
    ret = elf_lookup_shdr(&l->elf, ".got", SHT_PROGBITS, &got_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *ro_sec_header;
    ret = elf_lookup_shdr(&l->elf, ".rodata", SHT_PROGBITS, &ro_sec_header);
    if (ret)
        return LINKO_ERR;

    /* FIXME: can we naively rely on the fixed section order? */
    size_t sz = (got_sec_header->sh_addr - plt_sec_header->sh_addr);
    l->map_sz = ALIGN_UP(sz, sysconf(_SC_PAGESIZE));
    l->map_region = mmap(NULL, l->map_sz, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (l->map_region == MAP_FAILED)
        return LINKO_ERR;

    l->plt_region = l->map_region;
    l->text_region =
        l->map_region + (text_sec_header->sh_addr - plt_sec_header->sh_addr);
    l->ro_region =
        l->map_region + (ro_sec_header->sh_addr - plt_sec_header->sh_addr);
    l->got_region =
        l->map_region + (got_sec_header->sh_addr - plt_sec_header->sh_addr);

    linko_copy_section(l, l->plt_region, plt_sec_header);
    linko_copy_section(l, l->text_region, text_sec_header);
    linko_copy_section(l, l->ro_region, ro_sec_header);
    linko_copy_section(l, l->got_region, got_sec_header);

    return LINKO_NO_ERR;
}

static int linko_protect_map(linko_t *l)
{
    if (mprotect(l->map_region, l->map_sz, PROT_READ | PROT_EXEC))
        return LINKO_ERR;
    return LINKO_NO_ERR;
}

static void *linko_dlsym(linko_t *l, char *symbol)
{
    /* TODO: recycle the resource */
    void *lib = dlopen(NULL, RTLD_LAZY);
    return dlsym(lib, symbol);
}

static int linko_do_rela(linko_t *l)
{
    uint8_t *elf_data = l->elf.inner->data;
    Elf64_Ehdr *elf_header = l->elf.inner->header;

    Elf64_Shdr *got_sec_header;
    int ret = elf_lookup_shdr(&l->elf, ".got", SHT_PROGBITS, &got_sec_header);
    if (ret)
        return LINKO_ERR;

    Elf64_Shdr *rela_sec_header;
    ret = elf_lookup_shdr(&l->elf, ".rela.plt", SHT_RELA, &rela_sec_header);
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
            char *f_symbol =
                strtab + symtab[ELF64_R_SYM(relatab[i].r_info)].st_name;

            void *addr = linko_dlsym(l, f_symbol);
            *(uintptr_t *) (l->got_region +
                            (relatab[i].r_offset - got_sec_header->sh_addr)) =
                (uintptr_t) addr;
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
    int ret = elf_lookup_shdr(&l->elf, ".text", SHT_PROGBITS, &text_sec_header);
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
