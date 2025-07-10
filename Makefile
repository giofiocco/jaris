TARGETS=assembler linker encodemem inspect test sim encodefont
CFLAGS=-Wall -Wextra -g -std=c99

.PHONY: all asm docs clean cleanall
all: $(TARGETS) asm docs
asm: font
	make -C asm all
docs:
	make -C docs all

assembler: src/assembler.c src/assemble.c src/files.c src/files.h src/instructions.c src/instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

linker: src/linker.c src/link.c src/link.h src/files.c src/files.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

encodemem: src/encodemem.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

inspect: src/inspect.c src/files.c src/files.h src/instructions.c src/instructions.h
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test: src/test.c assembler linker
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

sim: src/sim.c src/files.c src/files.h src/instructions.c src/instructions.h main.mem.bin test.mem.bin
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) -lraylib

font: encodefont
	./encodefont $@

encodefont: src/encodefont.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

%.mem.bin: %.mem asm encodemem
	./encodemem $<

clean:
	rm -f $(TARGETS)
	rm -f *.bin
	rm -f font

cleanall: clean
	make -C asm clean
	make -C docs clean
