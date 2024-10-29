TARGETS=assembler linker encodemem sim inspect decodemem makemddocs
CFLAGS=-Wall -Wextra -Werror -std=c99

STDLIB_FILES=mul div solve_path open_file read_file execute exit put_char print get_char get_delim
PROGRAMS=shutdown ls sh
MEM_FILES=__bootloader __os __stdlib $(PROGRAMS)

all: $(TARGETS)

.PHONY: clean

mem.bin: $(patsubst %,mem/%,$(MEM_FILES)) encodemem | mem
	./encodemem

asm/build:
	mkdir -p $@

mem:
	mkdir -p $@

asm/build/%.o: asm/%.asm assembler | asm/build 
	./assembler -o $@ $<

mem/__bootloader: asm/build/bootloader.o linker | mem
	./linker --bin --nostdlib -o $@ $<
	wc -c $@ 

mem/__os: asm/build/os.o mem/__stdlib linker | mem
	./linker -o $@ $<

mem/__stdlib: $(patsubst %,asm/build/%.o,$(STDLIB_FILES)) linker | mem 
	./linker --so --nostdlib -o $@ $(filter %.o, $^) 

mem/shutdown: asm/build/shutdown.o mem/__stdlib linker | mem
	./linker -o $@ $<
mem/ls: asm/build/ls.o mem/__stdlib linker | mem
	./linker -o $@ $<
mem/sh: asm/build/sh.o mem/__stdlib linker | mem
	./linker -o $@ $<

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

decodemem: decodemem.c
	cc $(CFLAGS) -o $@ $<

makemddocs: makemddocs.c $(ARG_PARSER_LIB) 
	cc $(CFLAGS) -o $@ $(filter %.c, $^) 

inspect: inspect.c $(FILES_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB)
	cc $(CFLAGS) -o $@ $(filter %.c, $^)

sim: sim.c $(ARG_PARSER_LIB) $(SIM_DEP) mem.bin 
	cc $(CFLAGS) -o $@ $(filter %.c, $^)

clean:
	rm -r $(TARGETS) mem.bin asm/build
