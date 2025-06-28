TARGETS=assembler linker encodemem inspect test sim main.mem.bin
CFLAGS=-Wall -Wextra -g -std=c99

# TODO: add errors, argparse, sv ... dep

.PHONY: all asm clean
all: asm $(TARGETS)
asm:
	make -C asm all

assembler: assembler.c assemble.c files.c files.h ../instructions.c ../instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

linker: linker.c link.c link.h files.c files.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

encodemem: encodemem.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

inspect: inspect.c files.c files.h ../instructions.c ../instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test: test.c assembler linker
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

sim: sim.c files.c files.h ../instructions.c ../instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) -lraylib

%.mem.bin: %.mem encodemem
	./encodemem $<

clean:
	rm $(TARGETS)
	make -C asm clean
