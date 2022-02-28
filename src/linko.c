#include "linko.h"
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include "elf_parser.h"
#include "err.h"

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

    if (elf_get_symbol(&l->elf, symbol)) {
        return LINKO_ERR;
    }

    return LINKO_NO_ERR;
}

void linko_close(linko_t *l)
{
    elf_close(&l->elf);
}
