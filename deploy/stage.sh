#!/usr/bin/env bash
# ==============================================================================
# BLU-fx Staging & Packaging Tool
# ------------------------------------------------------------------------------
# Assembles multi-channel SkunkCrafts staging payload for BLU-fx
# and runs xpdepot.py to compute depot manifests (whitelist, sizeslist, blacklist).
#
# Usage:
#   ./deploy/stage.sh [release|beta|daily|all] [--build-depot]
# ==============================================================================

set -euo pipefail

TARGET="${1:-all}"
BUILD_DEPOT=false

for arg in "$@"; do
    if [[ "$arg" == "--build-depot" ]]; then
        BUILD_DEPOT=true
    fi
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RELEASE_SRC="$REPO_ROOT/release/blu_fx"

stage_channel() {
    local CHANNEL="$1"
    local CHANNEL_CFG="$REPO_ROOT/deploy/channels/${CHANNEL}_config.txt"
    if [[ ! -f "$CHANNEL_CFG" ]]; then
        echo "Error: Unknown channel configuration: $CHANNEL_CFG"
        echo "Available channels: release, beta, daily, all"
        exit 1
    fi

    local STAGING_DIR="$REPO_ROOT/skunkcrafts/$CHANNEL/blu_fx"
    echo "==> Staging BLU-fx for channel: $CHANNEL"
    rm -rf "$STAGING_DIR"
    mkdir -p "$STAGING_DIR"

    # 1. Copy documentation and metadata
    echo "    Copying documentation & version metadata..."
    local RESOLVED_VER=$(grep '^version|' "$CHANNEL_CFG" | head -n1 | cut -d'|' -f2 | tr -d '\r')
    if [[ "$CHANNEL" != "release" ]]; then
        RESOLVED_VER="$BLUFX_VERSION"
    fi

    # Generate stamped README.txt with version, timestamp, and attribution
    {
        echo "BLU-fx -- Real-Time Color Grading for X-Plane 12 & 11"
        echo "Copyright (C) $(date +%Y) by Steven L. Goldberg (@slgoldberg)"
        echo "v${RESOLVED_VER} (packaged on $(date))"
        echo "See ReleaseNotes.txt for full change history."
        echo "==============================================================================="
        echo ""
        tail -n +4 "$RELEASE_SRC/README.txt"
    } > "$STAGING_DIR/README.txt"

    if [[ -f "$RELEASE_SRC/ReleaseNotes.txt" ]]; then
        cp -a "$RELEASE_SRC/ReleaseNotes.txt" "$STAGING_DIR/"
    fi
    if [[ -f "$REPO_ROOT/docs/BLUFX_User_Guide.pdf" ]]; then
        mkdir -p "$STAGING_DIR/docs"
        cp -a "$REPO_ROOT/docs/BLUFX_User_Guide.pdf" "$STAGING_DIR/docs/"
    fi
    echo "blu-fx_v${RESOLVED_VER}  (packaged on $(date))" > "$STAGING_DIR/Version.txt"

    # 2. Copy multiplatform binaries
    echo "    Assembling multiplatform binaries..."
    mkdir -p "$STAGING_DIR/mac_x64" "$STAGING_DIR/win_x64" "$STAGING_DIR/lin_x64"

    # Mac: check build/mac_x64/blu_fx.xpl first, fallback to release/blu_fx/mac_x64/blu_fx.xpl
    if [[ -f "$REPO_ROOT/build/mac_x64/blu_fx.xpl" ]]; then
        cp -a "$REPO_ROOT/build/mac_x64/blu_fx.xpl" "$STAGING_DIR/mac_x64/blu_fx.xpl"
        echo "    [Mac] Staged local build: build/mac_x64/blu_fx.xpl"
    elif [[ -f "$RELEASE_SRC/mac_x64/blu_fx.xpl" ]]; then
        cp -a "$RELEASE_SRC/mac_x64/blu_fx.xpl" "$STAGING_DIR/mac_x64/blu_fx.xpl"
        echo "    [Mac] Staged release baseline: release/blu_fx/mac_x64/blu_fx.xpl"
    fi

    # Windows: check build-win/win_x64/blu_fx.xpl first, fallback to release/blu_fx/win_x64/blu_fx.xpl
    if [[ -f "$REPO_ROOT/build-win/win_x64/blu_fx.xpl" ]]; then
        cp -a "$REPO_ROOT/build-win/win_x64/blu_fx.xpl" "$STAGING_DIR/win_x64/blu_fx.xpl"
        echo "    [Windows] Staged local build: build-win/win_x64/blu_fx.xpl"
    elif [[ -f "$RELEASE_SRC/win_x64/blu_fx.xpl" ]]; then
        cp -a "$RELEASE_SRC/win_x64/blu_fx.xpl" "$STAGING_DIR/win_x64/blu_fx.xpl"
        echo "    [Windows] Staged release baseline: release/blu_fx/win_x64/blu_fx.xpl"
    fi

    # Linux: check build-lin/lin_x64/blu_fx.xpl first, fallback to release/blu_fx/lin_x64/blu_fx.xpl
    if [[ -f "$REPO_ROOT/build-lin/lin_x64/blu_fx.xpl" ]]; then
        cp -a "$REPO_ROOT/build-lin/lin_x64/blu_fx.xpl" "$STAGING_DIR/lin_x64/blu_fx.xpl"
        echo "    [Linux] Staged local build: build-lin/lin_x64/blu_fx.xpl"
    elif [[ -f "$RELEASE_SRC/lin_x64/blu_fx.xpl" ]]; then
        cp -a "$RELEASE_SRC/lin_x64/blu_fx.xpl" "$STAGING_DIR/lin_x64/blu_fx.xpl"
        echo "    [Linux] Staged release baseline: release/blu_fx/lin_x64/blu_fx.xpl"
    fi

    # 3. Control files & channel configuration
    echo "    Copying control lists and channel configuration..."
    cp -a "$CHANNEL_CFG" "$STAGING_DIR/skunkcrafts_updater_config.txt"
    if [[ "$CHANNEL" != "release" ]]; then
        sed -E "s/^version\|.*/version|${BLUFX_VERSION}/" "$CHANNEL_CFG" > "$STAGING_DIR/skunkcrafts_updater_config.txt"
    fi
    cp -a "$RELEASE_SRC/skunkcrafts_updater_blacklist.txt" "$STAGING_DIR/"
    cp -a "$RELEASE_SRC/skunkcrafts_updater_ignorelist.txt" "$STAGING_DIR/"
    cp -a "$RELEASE_SRC/skunkcrafts_updater_oncelist.txt" "$STAGING_DIR/"

    echo "==> Staging complete at: $STAGING_DIR"

    # 4. Build depot manifests via xpdepot.py
    if [[ "$BUILD_DEPOT" == true ]]; then
        local DEPOT_DIR="$REPO_ROOT/build/depot/depot/$CHANNEL/blu_fx"
        echo "==> Generating depot manifests for: $CHANNEL"
        mkdir -p "$DEPOT_DIR"
        python3 "$REPO_ROOT/deploy/xpdepot.py" --stage "$STAGING_DIR" --depot "$DEPOT_DIR"
        touch "$REPO_ROOT/build/depot/.nojekyll"
        echo "==> Depot ready at: $DEPOT_DIR"
    fi
}

case "$TARGET" in
    release)
        stage_channel release
        ;;
    beta)
        stage_channel beta
        ;;
    daily)
        stage_channel daily
        ;;
    all)
        stage_channel release
        stage_channel beta
        stage_channel daily
        ;;
    *)
        echo "Usage: $0 [release|beta|daily|all] [--build-depot]"
        exit 1
        ;;
esac
