TARGETS=assembler linker encodemem sim inspect
ARG_PARSER=arg_parser.h/arg_parser.h

all: $(TARGETS)

.PHONY: clean

CFLAGS=-Wall -Wextra -Werror -std=c99

assemble.o: assemble.c 
	cc $(CFLAGS) -c $<

instructions.o: instructions.c instructions.h sv.h
	cc $(CFLAGS) -c $<

files.o: files.c files.h instructions.o errors.h sv.h 
	cc $(CFLAGS) -o $@ -c $< 

assembler: assembler.c assemble.o errors.h instructions.o files.o $(ARG_PARSER) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^) 

link.o: link.c
	cc $(CFLAGS) -c $<

linker: linker.c link.o files.o $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^) 

encodemem: encodemem.c
	cc $(CFLAGS) -D_DEFAULT_SOURCE -o $@ $(filter %.c, $^)

sim: sim.c mem.bin $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

MEM=$(shell find mem)
mem.bin: encodemem $(MEM)
	./encodemem

inspect: inspect.c files.o $(ARG_PARSER)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^)

clean:
	rm *.o
	rm $(TARGETS)
	mem.bin
