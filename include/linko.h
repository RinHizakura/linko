#ifndef LINKO_H
#define LINKO_H

#include <elf.h>
#include "elf_parser.h"

typedef struct {
    elf_t elf;
} linko_t;

int linko_init(linko_t *l, char *obj_file);
int linko_find_symbol(linko_t *l, char *symbol);
void linko_close(linko_t *l);

#endif
