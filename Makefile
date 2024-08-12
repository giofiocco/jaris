all: assembler

CFLAGS=-Wall -Wextra -Werror -std=c99 

assembler: assembler.c assemble.o errors.h files.h sv.h 
	cc $(CFLAGS) -DSV_IMPLEMENTATION -DERRORS_IMPLEMENTATION -o $@ $(filter %.c %.o, $^)

assemble.o: assemble.c 
	cc $(CFLAGS) -c $<
