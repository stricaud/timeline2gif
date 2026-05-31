#!/bin/sh
# Generate all sample output files.
# Run from the build/ directory:
#
#   cd build
#   ../generate-samples.sh

set -e

BIN="./src/timeline2gif"
SAMPLES="../samples"

if [ ! -x "$BIN" ]; then
    echo "Error: $BIN not found. Run 'make' inside build/ first." >&2
    exit 1
fi

run() {
    echo "  $1 -> $2"
    "$BIN" "$SAMPLES/$1" "$SAMPLES/$2"
}

echo "Generating samples..."
run first.tig       first.gif
run first.tig       first.webp
run custom.tig      custom.gif
run custom.tig      custom.webp
run transitions.tig transitions.gif
run transitions.tig transitions.webp
run threat.tig      threat.gif
run threat.tig      threat.webp

echo "Done. Output files:"
ls -lh "$SAMPLES"/*.gif "$SAMPLES"/*.webp 2>/dev/null
