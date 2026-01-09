#!/bin/bash

# Build Mermaid diagrams to PNG
# Requires: npm install -g @mermaid-js/mermaid-cli

set -e

echo "Building Mermaid diagrams..."

# Check if mmdc is installed
if ! command -v mmdc &> /dev/null; then
    echo "Error: mermaid-cli not found."
    echo "Install with: npm install -g @mermaid-js/mermaid-cli"
    exit 1
fi

# Create output directory
mkdir -p ../images

# Convert each .mmd file to PNG
for mmd_file in *.mmd; do
    if [ -f "$mmd_file" ]; then
        basename="${mmd_file%.mmd}"
        echo "  Converting $mmd_file → ../images/${basename}.png"
        mmdc -i "$mmd_file" -o "../images/${basename}.png" -w 1200 -b transparent
    fi
done

echo ""
echo "✓ All diagrams converted to images/"
echo ""
echo "Generated images:"
ls -lh ../images/*.png 2>/dev/null || echo "No images generated"
