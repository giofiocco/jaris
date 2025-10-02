PROGRAMS=os shutdown ls sh cd cat test_font echo brainfuck expr bfjit tee stack
STDLIB_FILES=math solve_path open_file read_file execute exit print get_char string write_file alloc dynamic_array
STDLIB=bin/stdlib
STDLIB_LOC=$(STDLIB)
LINKER_FLAGS=--stdlib-path $(STDLIB_LOC) -g
ASSEMBLER_FLAGS=-g
ASSEMBLER?=../assembler
LINKER?=../linker

.PHONY: all clean
all: $(patsubst %,bin/%,$(PROGRAMS)) $(STDLIB) bin/bootloader

build:
	mkdir -p $@

bin:
	mkdir -p $@

build/bfjit.o: bfjit.asm $(ASSEMBLER) | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -a inst-as-arg -o $@ $<

build/stack.o: stack.asm $(ASSEMBLER) | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -a inst-as-arg -o $@ $<

build/%.o: %.asm $(ASSEMBLER) | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -o $@ $<

$(STDLIB): $(patsubst %,build/%.o,$(STDLIB_FILES)) $(LINKER) | bin
	$(LINKER) $(LINKER_FLAGS) --so --nostdlib -o $@ $(filter %.o,$^)

bin/os: build/os.o build/load_font.o $(STDLIB) $(LINKER) | bin
	$(LINKER) $(LINKER_FLAGS) -o $@ $(filter %.o,$^)

bin/bootloader: build/bootloader.o $(LINKER) | bin
	$(LINKER) $(LINKER_FLAGS) --bin --nostdlib -o $@ $<
	wc -c $@

bin/%: build/%.o $(LINKER) $(STDLIB) | bin
	$(LINKER) $(LINKER_FLAGS) -o $@ $<

clean:
	rm -rf bin build
