TARGETS=assembler linker encodemem inspect test sim encodefont
CFLAGS=-Wall -Wextra -g -std=c99

# TODO: add errors, argparse, sv ... dep

.PHONY: all asm clean
all: $(TARGETS) asm
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

sim: sim.c files.c files.h ../instructions.c ../instructions.h main.mem.bin test.mem.bin
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) -lraylib

font: encodefont
	./encodefont $@

encodefont: encodefont.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

%.mem.bin: %.mem asm encodemem
	./encodemem $<

clean:
	rm $(TARGETS)
	rm *.bin
	make -C asm clean
