PDF_NAME=main
OUT_DIR=./out

LFLAGS=-file-line-error --shell-escape

.PHONY: create_dirs

all: create_dirs main.tex
	pdflatex $(LFLAGS) main
	# biber main
	# pdflatex main
	# pdflatex main
	# cp $(OUT_DIR)/$(PDF_NAME).pdf $(PDF_NAME).pdf
	# rm -r $(OUT_DIR)

create_dirs:
	mkdir -p $(OUT_DIR)
	cp biblio.bib $(OUT_DIR)

clean: *.pdf
	rm *.aux *.bcf *.dvi *.fdb_latexmk *.fls *.glo *.idx *.ind *.ist *.out *.log *.xml *.acn *.bbl *.blg *.glsdefs *.ilg *.acr *.alg *.glg *.gls *.toc
