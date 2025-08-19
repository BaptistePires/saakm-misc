PDF_NAME=main
OUT_DIR=./out

.PHONY: create_dirs

all: create_dirs main.tex
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	biber --output-directory $(OUT_DIR) --input-directory $(OUT_DIR) main
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	pdflatex -output-directory $(OUT_DIR) -jobname $(PDF_NAME) main
	cp $(OUT_DIR)/$(PDF_NAME).pdf $(PDF_NAME).pdf
	rm -r $(OUT_DIR)

create_dirs:
	mkdir -p $(OUT_DIR)
	cp biblio.bib $(OUT_DIR)

clean: *.pdf
	rm -rf $(PDF_NAME).pdf $(OUT_DIR)
