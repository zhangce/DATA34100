#!/bin/bash
# Build script for lecture8_simd slides
# Preprocesses TikZ for beamer (removes Obsidian TikZJax wrapper)

INPUT="${1:-lecture-simd.md}"
OUTPUT="${INPUT%.md}.pdf"

# Remove ```tikz fences, \begin{document}, \end{document}
# Also handle the closing ``` after \end{tikzpicture}
sed -e '/^```tikz$/d' \
    -e '/^\\begin{document}$/d' \
    -e '/^\\end{document}$/d' \
    "$INPUT" | \
sed -e '/^\\end{tikzpicture}$/{N;s/\n```$//;}' | \
pandoc -t beamer --pdf-engine=xelatex -o "$OUTPUT"

echo "Built: $OUTPUT"
