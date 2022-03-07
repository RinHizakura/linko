#include "linko.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "elf_parser.h"
#include "err.h"
#include "utils/align.h"

int linko_init(linko_t *l, char *obj_file)
{
    int fd = open(obj_file, O_RDONLY);
    if (fd < 0)
        return LINKO_ERR;

    int ret = elf_init(&l->elf, fd);
    close(fd);

    if (ret)
        return LINKO_ERR;

    return LINKO_NO_ERR;
}

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

    l->text_sz = ALIGN_UP(text_sec_header.sh_size, sysconf(_SC_PAGESIZE));
    l->text_region = mmap(NULL, l->text_sz, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (l->text_region == MAP_FAILED)
        return NULL;

    /* copy the contents of `.text` section from the ELF file */
    elf_copy_section(&l->elf, &text_sec_header, l->text_region);

    if (mprotect(l->text_region, l->text_sz, PROT_READ | PROT_EXEC))
        return NULL;

    return (void *) (l->text_region + (sym.st_value - text_sec_header.sh_addr));
}

void linko_close(linko_t *l)
{
    munmap(l->text_region, l->text_sz);
    elf_close(&l->elf);
}
