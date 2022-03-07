#ifndef LINKO_H
#define LINKO_H

#include <elf.h>
#include "elf_parser.h"

typedef struct {
    elf_t elf;
    uint8_t *text_region;
    size_t text_sz;
} linko_t;

int linko_init(linko_t *l, char *obj_file);
void *linko_find_symbol(linko_t *l, char *symbol);
void linko_close(linko_t *l);

#endif
