CFLAGS=-Wall -Wextra -g -std=c99

.PHONY: all
all: assembler linker

assembler: assembler.c assemble.c files.c files.h ../instructions.c ../instructions.h
	cc $(CFLAGS) -o $@ $(filter %.c,$^)

linker: linker.c link.c link.h files.c files.h
	cc $(CFLAGS) -o $@ $(filter %.c,$^)

.PHONY: test
test:
	perl ../test.pl $(wildcard tests/*.test)
