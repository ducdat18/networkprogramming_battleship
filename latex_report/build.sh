#!/bin/bash

# Build script for LaTeX report
# Usage: ./build.sh

set -e

echo "======================================"
echo "Building Battleship LaTeX Report"
echo "======================================"

# Check if pdflatex is installed
if ! command -v pdflatex &> /dev/null; then
    echo "Error: pdflatex not found. Please install texlive-full:"
    echo "  sudo apt-get install texlive-full"
    exit 1
fi

# Create build directory
mkdir -p build

# Run pdflatex (first pass)
echo ""
echo "[1/3] Running pdflatex (first pass)..."
pdflatex -output-directory=build -interaction=nonstopmode main.tex > /dev/null

# Run pdflatex (second pass for references)
echo "[2/3] Running pdflatex (second pass for references)..."
pdflatex -output-directory=build -interaction=nonstopmode main.tex > /dev/null

# Run pdflatex (third pass for table of contents)
echo "[3/3] Running pdflatex (third pass for TOC)..."
pdflatex -output-directory=build -interaction=nonstopmode main.tex > /dev/null

# Move final PDF to root
cp build/main.pdf battleship_report_person2.pdf

echo ""
echo "======================================"
echo "✓ Build successful!"
echo "======================================"
echo "Output: battleship_report_person2.pdf"
echo ""

# Show warnings if any
if grep -q "Warning" build/main.log 2>/dev/null; then
    echo "⚠ Warnings found (check build/main.log):"
    grep "Warning" build/main.log | head -5
    echo ""
fi

# Open PDF if evince is available
if command -v evince &> /dev/null; then
    echo "Opening PDF..."
    evince battleship_report_person2.pdf &
elif command -v xdg-open &> /dev/null; then
    echo "Opening PDF..."
    xdg-open battleship_report_person2.pdf &
fi
