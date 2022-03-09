#ifndef LINKO_H
#define LINKO_H

#include <elf.h>
#include "elf_parser.h"
#include "type.h"

typedef struct {
    elf_t elf;
    TYPE type;
    uint8_t *plt_region;
    uint8_t *text_region;
    uint8_t *map_region;
    size_t map_sz;
} linko_t;

int linko_init(linko_t *l, char *obj_file, TYPE file_type);
void *linko_find_symbol(linko_t *l, char *symbol);
void linko_close(linko_t *l);

#endif
