TARGETS=assembler linker encodemem inspect sim encodefont code_analyzer
CFLAGS=-Wall -Wextra -g -std=c99

FILES=src/files.c src/files.h
INSTRUCTIONS=src/instructions.c src/instructions.h
ASSEMBLE=src/build/libassemble.a

.PHONY: all asm docs clean cleanall
all: $(TARGETS) asm docs
asm: font
	make -C asm all
docs:
	make -C docs all

src/build/:
	mkdir -p src/build

src/build/libassemble.a: src/build/assemble.o src/build/instructions.o | src/build/
	ar rcs $@ $(filter %.o,$^)

assembler: src/assembler.c $(ASSEMBLE) $(FILES)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) -L./src/build/ -lassemble

linker: src/linker.c src/link.c src/link.h $(FILES) $(INSTRUCTIONS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

encodemem: src/encodemem.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

inspect: src/inspect.c $(FILES) $(INSTRUCTIONS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

sim: src/sim.c main.mem.bin test.mem.bin $(FILES) $(INSTRUCTIONS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) -lraylib

code_analyzer: src/code_analyzer.c $(ASSEMBLE)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

font: encodefont
	./encodefont $@

encodefont: src/encodefont.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

%.mem.bin: %.mem asm encodemem
	./encodemem $<

src/build/%.o: src/%.c src/%.h | src/build/
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGETS)
	rm -f *.bin
	rm -f font
	rm -rf src/build/

cleanall: clean
	make -C asm clean
	make -C docs clean
