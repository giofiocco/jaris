TARGETS=assembler linker encodemem sim inspect
CFLAGS=-Wall -Wextra -Werror -std=c99

all: $(TARGETS)

.PHONY: clean

mem.bin: encodemem mem/__bootloader mem/stdlib mem/os $(wildcard mem/*) | mem
	./encodemem

asm/build:
	mkdir $@

mem:
	mkdir $@

asm/build/%.o: asm/%.asm assembler | asm/build 
	./assembler -o $@ $<

mem/__bootloader: asm/build/bootloader.o linker | mem
	./linker --bin --nostdlib -o $@ $<
	wc -c $@ 

mem/os: asm/build/os2.o mem/stdlib linker | mem
	./linker -o $@ $<

mem/stdlib: asm/build/mul.o linker | mem 
	./linker --so --nostdlib -o $@ $(filter %.o, $^) 

ARG_PARSER_LIB=argparse/argparse.c argparse/argparse.h
SV_LIB=mystb/sv.h
ERRORS_LIB=errors.c errors.h
FILES_DEP=files.c files.h $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 
INSTRUCTIONS_DEP=instructions.c instructions.h $(SV_LIB)
ASSEMBLE_DEP=assemble.c assemble.h $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 
SIM_DEP=instructions.c instructions.h

assembler: assembler.c $(ASSEMBLE_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB) 
	cc $(CFLAGS) -o $@ $(filter %.c, $^) 

linker: linker.c link.c $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ARG_PARSER_LIB) 
	cc $(CFLAGS) -o $@ $(filter %.c, $^) 

encodemem: encodemem.c
	cc $(CFLAGS) -D_DEFAULT_SOURCE -o $@ $(filter %.c, $^)

inspect: inspect.c $(FILES_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB)
	cc $(CFLAGS) -o $@ $(filter %.c, $^)

sim: sim.c $(ARG_PARSER_LIB) $(SIM_DEP) mem.bin 
	cc $(CFLAGS) -o $@ $(filter %.c, $^)

clean:
	rm -r $(TARGETS) mem.bin asm/build mem
