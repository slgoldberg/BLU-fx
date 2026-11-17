#!/usr/bin/env bash
# ==============================================================================
# deploy/package.sh - Package BLU-fx release distribution ZIP archive
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 1. Resolve version from blu_fx.h
BLUFX_VERSION=$(grep '#define VERSION' "$REPO_ROOT/blu_fx.h" | head -n1 | sed -E 's/.*"([^"]+)".*/\1/')
if [[ -z "$BLUFX_VERSION" ]]; then
    echo "ERROR: Could not resolve VERSION from $REPO_ROOT/blu_fx.h" >&2
    exit 1
fi
echo "==> Packaging BLU-fx v${BLUFX_VERSION}..."

COPY_TO_DOWNLOADS=false
for arg in "$@"; do
    case "$arg" in
        --copy-to-downloads)
            COPY_TO_DOWNLOADS=true
            ;;
        *)
            ;;
    esac
done

DIST_ROOT="$REPO_ROOT/build/dist"
DIST_DIR="$DIST_ROOT/blu_fx"
ZIP_NAME="blu_fx_v${BLUFX_VERSION}.zip"
ZIP_PATH="$DIST_ROOT/$ZIP_NAME"

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/docs" "$DIST_DIR/mac_x64" "$DIST_DIR/win_x64" "$DIST_DIR/lin_x64"

# 2. Copy documentation & control files
echo "    Copying documentation and control files..."
cp -a "$REPO_ROOT/gpl-2.0.txt" "$DIST_DIR/"
cp -a "$REPO_ROOT/release/blu_fx/ReleaseNotes.txt" "$DIST_DIR/"

# Generate stamped README.txt with version, timestamp, and attribution
{
    echo "BLU-fx -- Real-Time Color Grading for X-Plane 12 & 11"
    echo "Copyright (C) $(date +%Y) by Steven L. Goldberg (@slgoldberg)"
    echo "v${BLUFX_VERSION} (packaged on $(date))"
    echo "See ReleaseNotes.txt for full change history."
    echo "==============================================================================="
    echo ""
    tail -n +4 "$REPO_ROOT/release/blu_fx/README.txt"
} > "$DIST_DIR/README.txt"

if [[ -f "$REPO_ROOT/docs/BLUFX_User_Guide.pdf" ]]; then
    cp -a "$REPO_ROOT/docs/BLUFX_User_Guide.pdf" "$DIST_DIR/docs/"
fi

# Generate Version.txt
echo "blu-fx_v${BLUFX_VERSION}  (packaged on $(date))" > "$DIST_DIR/Version.txt"

# Stamp version into skunkcrafts_updater.cfg prototype
sed -E "s/^version\|.*/version|${BLUFX_VERSION}/" "$REPO_ROOT/release/blu_fx/skunkcrafts_updater.cfg" > "$DIST_DIR/skunkcrafts_updater.cfg"

# 3. Copy multiplatform binaries
# macOS
if [[ -f "$REPO_ROOT/build/mac_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/build/mac_x64/blu_fx.xpl" "$DIST_DIR/mac_x64/"
elif [[ -f "$REPO_ROOT/release/blu_fx/mac_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/release/blu_fx/mac_x64/blu_fx.xpl" "$DIST_DIR/mac_x64/"
fi

# Windows
if [[ -f "$REPO_ROOT/build-win/win_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/build-win/win_x64/blu_fx.xpl" "$DIST_DIR/win_x64/"
elif [[ -f "$REPO_ROOT/build/win_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/build/win_x64/blu_fx.xpl" "$DIST_DIR/win_x64/"
elif [[ -f "$REPO_ROOT/release/blu_fx/win_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/release/blu_fx/win_x64/blu_fx.xpl" "$DIST_DIR/win_x64/"
fi

# Linux
if [[ -f "$REPO_ROOT/build-lin/lin_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/build-lin/lin_x64/blu_fx.xpl" "$DIST_DIR/lin_x64/"
elif [[ -f "$REPO_ROOT/build/lin_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/build/lin_x64/blu_fx.xpl" "$DIST_DIR/lin_x64/"
elif [[ -f "$REPO_ROOT/release/blu_fx/lin_x64/blu_fx.xpl" ]]; then
    cp -a "$REPO_ROOT/release/blu_fx/lin_x64/blu_fx.xpl" "$DIST_DIR/lin_x64/"
fi

# 4. Remove any transient OS files
find "$DIST_DIR" -name ".DS_Store" -delete 2>/dev/null || true

# 5. Create ZIP package
echo "    Creating $ZIP_NAME..."
rm -f "$ZIP_PATH"
(cd "$DIST_ROOT" && zip -rq "$ZIP_NAME" blu_fx -x "*.DS_Store" "*/.gitkeep")

echo "==> Package created successfully:"
ls -lh "$ZIP_PATH"

# 6. Optional: Copy to ~/Downloads
if [[ "$COPY_TO_DOWNLOADS" == true && -d "$HOME/Downloads" ]]; then
    echo "    Copying package to $HOME/Downloads/..."
    cp -a "$ZIP_PATH" "$HOME/Downloads/"
    echo "    Copied to $HOME/Downloads/$ZIP_NAME"
fi
