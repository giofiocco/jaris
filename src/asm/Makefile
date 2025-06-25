PROGRAMS=os shutdown ls sh cd cat test_font echo brainfuck expr
STDLIB_FILES=math solve_path open_file read_file execute exit print get_char string
STDLIB=bin/stdlib
STDLIB_LOC=$(STDLIB)
LINKER_FLAGS=--stdlib-path $(STDLIB_LOC) -g
ASSEMBLER_FLAGS=-g

.PHONY: all clean
all: $(patsubst %,bin/%,$(PROGRAMS)) $(STDLIB) bin/bootloader

build:
	mkdir -p $@

bin:
	mkdir -p $@

build/%.o: %.asm ../assembler | build
	../assembler $(ASSEMBLER_FLAGS) -o $@ $<

$(STDLIB): $(patsubst %,build/%.o,$(STDLIB_FILES)) ../linker | bin
	../linker $(LINKER_FLAGS) --so --nostdlib -o $@ $(filter %.o,$^)

bin/os: build/os.o build/load_font.o $(STDLIB) ../linker | bin
	../linker $(LINKER_FLAGS) -o $@ $(filter %.o,$^)

bin/bootloader: build/bootloader.o ../linker | bin
	../linker $(LINKER_FLAGS) --bin --nostdlib -o $@ $<
	wc -c $@

bin/%: build/%.o ../linker $(STDLIB) | bin
	../linker $(LINKER_FLAGS) -o $@ $<


../linker:
	make -C .. linker

../assembler:
	make -C .. assembler

clean:
	rm -r bin build
