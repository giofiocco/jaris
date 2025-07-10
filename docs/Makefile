.PHONY: all clean
all: $(patsubst %.typ,%.pdf,$(wildcard *.typ)) $(patsubst %.roff,%.pdf,$(wildcard *.roff))

%.pdf: %.typ
	typst c $< --root ..

%.pdf: %.roff
	groff -p -t -ms $^ -Tpdf > $@

clean:
	rm -f *.pdf
