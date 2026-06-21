#!/usr/bin/env bash
# Install Timeline Studio for the current user and associate .tig files with it.
# Run from the extracted tarball directory (the one containing `Timeline`).
#
#   ./packaging/linux/install.sh
#
# Undo with: ./packaging/linux/install.sh --uninstall
set -euo pipefail

PREFIX="${XDG_DATA_HOME:-$HOME/.local/share}"
BIN="$HOME/.local/bin"
APPS="$PREFIX/applications"
MIME="$PREFIX/mime"
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"   # tarball root (where Timeline lives)

if [[ "${1:-}" == "--uninstall" ]]; then
    rm -f "$BIN/Timeline" "$BIN/timeline2gif" \
          "$APPS/timeline2gif.desktop" \
          "$MIME/packages/timeline2gif.xml"
    command -v update-mime-database     >/dev/null && update-mime-database "$MIME" || true
    command -v update-desktop-database  >/dev/null && update-desktop-database "$APPS" || true
    echo "Removed Timeline Studio and its .tig association."
    exit 0
fi

mkdir -p "$BIN" "$APPS" "$MIME/packages"

# Binaries (Exec=Timeline in the .desktop resolves via PATH).
install -m 0755 "$ROOT/Timeline"      "$BIN/Timeline"
install -m 0755 "$ROOT/timeline2gif"  "$BIN/timeline2gif"

# MIME type + desktop entry.
install -m 0644 "$HERE/timeline2gif.xml"     "$MIME/packages/timeline2gif.xml"
install -m 0644 "$HERE/timeline2gif.desktop" "$APPS/timeline2gif.desktop"

# Refresh the databases and set Timeline Studio as the default .tig handler.
command -v update-mime-database    >/dev/null && update-mime-database "$MIME" || true
command -v update-desktop-database >/dev/null && update-desktop-database "$APPS" || true
command -v xdg-mime                >/dev/null && \
    xdg-mime default timeline2gif.desktop application/x-tig || true

echo "Installed. Make sure $BIN is on your PATH, then double-click a .tig file."
