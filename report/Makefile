# Build the report. Two passes, so figure and table references resolve.
#   make        build the PDF
#   make check  build, then report anything still unmeasured
#   make clean  remove build output

TEX = prac_02_lxxtri004_shrgil004

all: $(TEX).pdf

$(TEX).pdf: $(TEX).tex measurements.tex $(wildcard figures/*)
	pdflatex -interaction=nonstopmode $(TEX).tex > /dev/null
	pdflatex -interaction=nonstopmode $(TEX).tex > /dev/null
	@grep -c Overfull $(TEX).log | xargs -I{} echo "overfull boxes: {}"
	@echo "pages: $$(pdfinfo $(TEX).pdf | awk '/Pages/{print $$2}')"

# The completion test: nothing unmeasured, and every figure present.
check: $(TEX).pdf
	@echo
	@n=$$(grep -c '^.newcommand.*.bench{' measurements.tex); \
	 if [ "$$n" -gt 0 ]; then \
	   echo "$$n measurement(s) still unfilled in measurements.tex:"; \
	   grep -n '^.newcommand.*.bench{' measurements.tex \
	     | sed -e 's/{.bench.*//' -e 's/.newcommand{.//' -e 's/^/  /'; \
	 else echo "all measurements filled"; fi
	@echo
	@for f in task1-pc13.png task3-sck-mosi.png task4-cs-sck.png \
	          task5-faulty.png task5-corrected.png; do \
	   [ -f figures/$$f ] || echo "  missing figure: figures/$$f"; \
	 done; echo "figure check done"
	@echo
	@echo "Report is ready only when both lists above are empty."

clean:
	rm -f $(TEX).pdf $(TEX).aux $(TEX).log $(TEX).out page-*.png chk-*.png

.PHONY: all check clean
