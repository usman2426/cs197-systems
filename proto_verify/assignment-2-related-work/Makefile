.PHONY: all main.pdf clean

main.pdf:
	latexmk main -outdir=build -pdflatex

clean:
	rm -rf *.fdb_latexmk *.fls *.log *.out *.aux main.pdf
