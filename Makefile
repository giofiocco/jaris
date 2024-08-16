all: assembler

.PHONY: clean

CFLAGS=-Wall -Wextra -Werror -std=c99 

assembler: assembler.c assemble.o errors.h files.o sv.h instructions.o 
	cc $(CFLAGS) -DSV_IMPLEMENTATION -DERRORS_IMPLEMENTATION -o $@ $(filter %.c %.o, $^)

assemble.o: assemble.c 
	cc $(CFLAGS) -c $<

instructions.o: instructions.c instructions.h
	cc $(CFLAGS) -c $<

files.o: files.c files.h
	cc $(CFLAGS) -c $<

clean:
	rm *.o
	rm assembler
