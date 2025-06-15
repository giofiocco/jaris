CFLAGS=-Wall -Wextra -g -std=c99

all: assembler linker

assembler: assembler.c assemble.c ../files.c ../files.h ../instructions.c ../instructions.h
	cc $(CFLAGS) -o $@ $(filter %.c,$^)

linker: linker.c ../link.c ../link.h ../files.c ../files.h ../errors.c ../errors.h
	cc $(CFLAGS) -o $@ $(filter %.c,$^)
