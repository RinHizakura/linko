CFLAGS = -Iinclude -Wall -Wextra #-Werror
LDFLAGS = -Wl,-rpath="$(CURDIR)" -L. -llinko

CURDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUT ?= build
LIBLINKO = liblinko.so
SHELL_HACK := $(shell mkdir -p $(OUT))

GIT_HOOKS := .git/hooks/applied

LIBSRCS = $(shell find ./lib -name '*.c')
_LIB_OBJ =  $(notdir $(LIBSRCS))
LIB_OBJ = $(_LIB_OBJ:%.c=$(OUT)/%.o)

CSRCS = $(shell find ./tests -name '*.c')
_COBJ =  $(notdir $(CSRCS))
COBJ = $(_COBJ:%.c=$(OUT)/%.o)

TESTS = $(OUT)/main $(OUT)/gcd

vpath %.c $(sort $(dir $(CSRCS)))
vpath %.c $(sort $(dir $(LIBSRCS)))

all: $(GIT_HOOKS) $(LIBLINKO) $(TESTS)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(OUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBLINKO): $(LIB_OBJ)
	$(CC) -shared $(LIB_OBJ) -o $@

$(TESTS): %: %.o $(LIBLINKO)
	$(CC) $^ -o $@ $(LDFLAGS)

check: all
	$(OUT)/main

clean:
	$(RM) $(LIB_OBJ)
	$(RM) $(COBJ)
	$(RM) $(TESTS)
	$(RM) $(LIBLINKO)
