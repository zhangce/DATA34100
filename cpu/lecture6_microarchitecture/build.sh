#!/bin/bash

# Build slides from markdown to PDF using pandoc + beamer

INPUT="${1:-lecture-cpu-microarchitecture.md}"
OUTPUT="${INPUT%.md}.pdf"

# Preprocess tikz blocks for beamer:
# - Remove ```tikz and closing ```
# - Remove \begin{document} and \end{document}
# (Obsidian TikZJax needs these, but beamer doesn't)
sed -e '/^```tikz$/d' \
    -e '/^\\begin{document}$/d' \
    -e '/^\\end{document}$/d' \
    "$INPUT" | \
sed -e '/^\\end{tikzpicture}$/{N;s/\n```$//;}' | \
pandoc \
    -t beamer \
    --pdf-engine=xelatex \
    -o "$OUTPUT"

echo "Built: $OUTPUT"
