PDF_NAME=saakm
OUT_DIR=./out

.PHONY: create_dirs

all: create_dirs main.tex
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	# bibtex $(OUT_DIR)/main
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	cp $(OUT_DIR)/$(PDF_NAME).pdf $(PDF_NAME).pdf
	rm -r $(OUT_DIR)

create_dirs:
	mkdir -p $(OUT_DIR)

clean: *.pdf
	rm $(PDF_NAME).pdf
