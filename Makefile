TARGETS=assembler linker encodemem sim inspect
ARG_PARSER_LIB=mystb/arg_parser.h
SV_LIB=mystb/sv.h
ERRORS_LIB=mystb/errors.h

all: $(TARGETS)

.PHONY: clean

CFLAGS=-Wall -Wextra -Werror -std=c99

build:
	mkdir build

MEM=$(shell find mem)
mem.bin: encodemem $(MEM)
	./encodemem

FILES_DEP=files.c files.h $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 
INSTRUCTIONS_DEP=instructions.c instructions.h $(SV_LIB)
ASSEMBLE_DEP=assemble.c assemble.h $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ERRORS_LIB) 

assembler: assembler.c $(ASSEMBLE_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^) 

linker: linker.c link.c $(FILES_DEP) $(INSTRUCTIONS_DEP) $(ARG_PARSER_LIB) 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^) 

encodemem: encodemem.c
	cc $(CFLAGS) -D_DEFAULT_SOURCE -o $@ $(filter %.c, $^)

sim: sim.c mem.bin $(ARG_PARSER_LIB)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

inspect: inspect.c $(FILES_DEP) $(ARG_PARSER_LIB) $(ERRORS_LIB)
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c, $^)

clean:
	rm -rf$(TARGETS) mem.bin
