#include "linko.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "elf_parser.h"
#include "utils/align.h"

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

    return LINKO_NO_ERR;
}

/* This is a set of x86 instructions which can hang the execution */
#include <string.h>
static uint8_t hang[10] = {0xf3, 0x0f, 0x1e, 0xfa, 0x55,
                           0x48, 0x89, 0xe5, 0xeb, 0xfe};

void *linko_find_symbol(linko_t *l, char *symbol)
{
    if (elf_check_valid(&l->elf))
        return NULL;

    Elf64_Sym sym;
    if (elf_lookup_function(&l->elf, symbol, &sym))
        return NULL;

    Elf64_Shdr text_sec_header;
    int ret = elf_lookup_section_hdr(&l->elf, ".text", SHT_PROGBITS,
                                     &text_sec_header);
    if (ret)
        return NULL;

    Elf64_Shdr plt_sec_header;
    ret = elf_lookup_section_hdr(&l->elf, ".plt.sec", SHT_PROGBITS,
                                 &plt_sec_header);
    if (ret)
        return NULL;

    size_t sz = plt_sec_header.sh_size + text_sec_header.sh_size;
    l->map_sz = ALIGN_UP(sz, sysconf(_SC_PAGESIZE));
    l->map_region = mmap(NULL, l->map_sz, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (l->map_region == MAP_FAILED)
        return NULL;

    l->plt_region = l->map_region;
    l->text_region = l->map_region + plt_sec_header.sh_size;
    memcpy(l->plt_region, hang, 10);

    /* copy the contents of `.text` section from the ELF file */
    elf_copy_section(&l->elf, &text_sec_header, l->text_region);

    if (mprotect(l->map_region, l->map_sz, PROT_READ | PROT_EXEC))
        return NULL;

    return (void *) (l->text_region + (sym.st_value - text_sec_header.sh_addr));
}

void linko_close(linko_t *l)
{
    munmap(l->map_region, l->map_sz);
    elf_close(&l->elf);
}
