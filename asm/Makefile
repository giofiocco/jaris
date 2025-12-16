PROGRAMS=os shutdown ls sh cd cat test_font echo brainfuck bfjit tee stack rule110 pipe clear rpncalc
STDLIB_FILES=math solve_path open_file read_file execute exit print get_char string write_file alloc dynamic_array rng stream
STDLIB=bin/stdlib
STDLIB_LOC=$(STDLIB)
LINKER_FLAGS?=-g
ASSEMBLER_FLAGS?=-g
ASSEMBLER=../assembler
LINKER=../linker --stdlib-path $(STDLIB_LOC)
ANALYZE?=../code_analyzer --stdlib-path $(STDLIB_LOC) $@

.PHONY: all clean
all: $(patsubst %,bin/%,$(PROGRAMS)) $(STDLIB) bin/bootloader

build:
	mkdir -p $@

bin:
	mkdir -p $@

build/bfjit.o: bfjit.asm | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -a inst-as-arg -o $@ $<
	$(ANALYZE)

build/stack.o: stack.asm | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -a inst-as-arg -o $@ $<
	$(ANALYZE)

build/%.o: %.asm | build
	$(ASSEMBLER) $(ASSEMBLER_FLAGS) -o $@ $<
	$(ANALYZE)

$(STDLIB): $(patsubst %,build/%.o,$(STDLIB_FILES)) | bin
	$(LINKER) $(LINKER_FLAGS) --so --nostdlib -o $@ $(filter %.o,$^)
	$(ANALYZE)

bin/os: build/os.o build/load_font.o $(STDLIB) | bin
	$(LINKER) $(LINKER_FLAGS) -o $@ $(filter %.o,$^)
	$(ANALYZE)

bin/bootloader: build/bootloader.o | bin
	$(LINKER) $(LINKER_FLAGS) --bin --nostdlib -o $@ $<
	wc -c $@
	$(ANALYZE) --bin

bin/%: build/%.o $(STDLIB) | bin
	$(LINKER) $(LINKER_FLAGS) -o $@ $<
	$(ANALYZE)

clean:
	rm -rf bin build
