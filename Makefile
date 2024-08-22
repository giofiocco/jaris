TARGETS=assembler linker encodemem sim inspect
ARG_PARSER=arg_parser.h/arg_parser.h

all: $(TARGETS)

.PHONY: clean

CFLAGS=-Wall -Wextra -Werror -std=c99

build:
	mkdir build

build/assemble.o: assemble.c build 
	cc $(CFLAGS) -o $@ -c $<

build/instructions.o: instructions.c instructions.h sv.h build
	cc $(CFLAGS) -o $@ -c $<

build/files.o: files.c files.h build/instructions.o errors.h sv.h build 
	cc $(CFLAGS) -o $@ -c $< 

build/link.o: link.c build
	cc $(CFLAGS) -o $@ -c $<

MEM=$(shell find mem)
mem.bin: encodemem $(MEM)
	./encodemem

assembler: assembler.c build/assemble.o errors.h build/instructions.o build/files.o $(ARG_PARSER) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^) 

linker: linker.c build/link.o build/files.o $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^) 

encodemem: encodemem.c
	cc $(CFLAGS) -D_DEFAULT_SOURCE -o $@ $(filter %.c, $^)

sim: sim.c mem.bin $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

inspect: inspect.c build/files.o $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^)

clean:
	rm -rf build/ $(TARGETS) mem.bin
