#include "linko.h"
#include <fcntl.h>
#include <stddef.h>
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

int linko_find_symbol(linko_t *l, char *symbol)
{
    if (elf_check_valid(&l->elf)) {
        return LINKO_ERR;
    }

    Elf32_Sym sym;
    if (elf_lookup_function(&l->elf, symbol, &sym)) {
        return LINKO_ERR;
    }

    Elf64_Shdr text_sec_header;
    int ret = elf_lookup_section_hdr(&l->elf, ".text", SHT_PROGBITS,
                                     &text_sec_header);
    if (ret)
        return LINKO_ERR;

    size_t region_sz = ALIGN_UP(text_sec_header.sh_size, sysconf(_SC_PAGESIZE));
    uint8_t *text_region = mmap(NULL, region_sz, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (text_region == MAP_FAILED)
        return LINKO_ERR;

    /* copy the contents of `.text` section from the ELF file */
    elf_copy_section(&l->elf, &text_sec_header, text_region);

    if (mprotect(text_region, region_sz, PROT_READ | PROT_EXEC))
        return LINKO_ERR;

    munmap(text_region, region_sz);

    return LINKO_NO_ERR;
}

void linko_close(linko_t *l)
{
    elf_close(&l->elf);
}
