TARGETS=assembler linker

all: $(TARGETS)

.PHONY: clean

CFLAGS=-Wall -Wextra -Werror -std=c99

assemble.o: assemble.c 
	cc $(CFLAGS) -c $<

instructions.o: instructions.c instructions.h sv.h
	cc $(CFLAGS) -c $<

files.o: files.c files.h instructions.o errors.h sv.h 
	cc $(CFLAGS) -o $@ -c $< 

assembler: assembler.c assemble.o errors.h instructions.o files.o 
	cc $(CFLAGS) -o $@ $(filter %.c %.o, $^) 

link.o: link.c
	cc $(CFLAGS) -c $<

linker: linker.c link.o files.o 
	cc $(CFLAGS) -DARG_PARSER_IMPLEMENTATION -o $@ $(filter %.c %.o, $^) 

clean:
	rm *.o
	rm $(TARGETS)
