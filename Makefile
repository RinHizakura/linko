CFLAGS = -Iinclude -Wall -Wextra #-Werror
LDFLAGS =

CURDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUT ?= build
BINARY = $(OUT)/linko
SHELL_HACK := $(shell mkdir -p $(OUT))

GIT_HOOKS := .git/hooks/applied

CSRCS = $(shell find ./src -name '*.c')
_COBJ =  $(notdir $(CSRCS))
COBJ = $(_COBJ:%.c=$(OUT)/%.o)

vpath %.c $(sort $(dir $(CSRCS)))

all: $(GIT_HOOKS) $(BINARY)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(OUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BINARY): $(COBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: tests/gcd.c
	$(CC) -c $(CFLAGS) $< -o $(OUT)/gcd.o

check: all test
	$(BINARY) $(OUT)/gcd.o gcd64

clean:
	$(RM) $(COBJ)
	$(RM) $(OUT)/gcd.o
	$(RM) $(BINARY)
