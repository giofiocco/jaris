.PHONY: all clean
all: aleph.pdf stdlibdoc.pdf insts_analysis.pdf

aleph.pdf: aleph.typ $(patsubst PIC/%.PIC,PIC/images/%.png,$(wildcard PIC/*.PIC))
	typst c $< --root ..

stdlibdoc.pdf: stdlibdoc.typ ../asm/Makefile
	typst c $< --root ..

%.pdf: %.typ
	typst c $< --root ..

%.pdf: %.roff
	groff -p -t -ms $^ -Tpdf > $@

PIC/images:
	mkdir -p $@

PIC/images/%.png: PIC/%.PIC | PIC/images
	@echo compiling $@...
	@printf ".PS\n%s\n.PE\n" "`cat $<`" \
		| groff -p -ms -Tps \
		| magick -density 200 - -trim -strip $@

clean:
	rm -f *.pdf
	rm -rf PIC/images
