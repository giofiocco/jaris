TARGETS=assembler linker encodemem sim inspect
CFLAGS=-Wall -Wextra -Werror -std=c99

all: $(TARGETS)

.PHONY: clean

mem.bin: encodemem mem/__bootloader mem/__os $(wildcard mem/*)
	./encodemem

asm/build:
	mkdir asm/build

asm/build/%.o: asm/%.asm assembler asm/build 
	./assembler -o $@ $<

mem/__bootloader: asm/build/bootloader.o linker
	./linker --bin -o $@ $<

mem/__os: asm/build/os.o linker
	./linker --dexe -o $@ $<

ARG_PARSER_LIB=argparse/argparse.c argparse/argparse.h
SV_LIB=mystb/sv.h
ERRORS_LIB=errors.c errors.h
FILES_DEP=files.c files.h $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 
INSTRUCTIONS_DEP=instructions.c instructions.h $(SV_LIB)
ASSEMBLE_DEP=assemble.c assemble.h $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 

assembler: assembler.c $(ASSEMBLE_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^) 

linker: linker.c link.c $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ARG_PARSER_LIB) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^) 

encodemem: encodemem.c
	cc $(CFLAGS) -D_DEFAULT_SOURCE -o $@ $(filter %.c, $^)

inspect: inspect.c $(FILES_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

sim: sim.c $(ARG_PARSER_LIB) mem.bin 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

clean:
	rm $(TARGETS) mem.bin
