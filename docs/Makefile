.PHONY: all clean
all: aleph.pdf aleph_images.pdf stdlibdoc.pdf insts_analysis.pdf

aleph.pdf: aleph.typ aleph_images.pl aleph_images.roff
	perl aleph_images.pl
	typst c $< --root ..

%.pdf: %.typ
	typst c $< --root ..

%.pdf: %.roff
	groff -p -t -ms $^ -Tpdf > $@

clean:
	rm -f *.pdf
	rm -rf aleph_images
