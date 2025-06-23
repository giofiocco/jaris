CFLAGS=-Wall -Wextra -g -std=c99
TESTS_DIR=tests

.PHONY: all
all: assembler linker test

assembler: assembler.c assemble.c files.c files.h ../instructions.c ../instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

linker: linker.c link.c link.h files.c files.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test: test.c assembler linker
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
	./$@
