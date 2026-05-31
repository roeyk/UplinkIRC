#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TOOLS_DIR="$SCRIPT_DIR/tools"
BUILD_DIR="$PROJECT_DIR/build-appimage"
APPDIR="$BUILD_DIR/AppDir"

# Read version from CMakeLists.txt
VERSION=$(grep -oP '(?<=project\(UplinkIRC VERSION )\S+' "$PROJECT_DIR/CMakeLists.txt")
ARCH=$(uname -m)

echo "==> UplinkIRC $VERSION  ($ARCH)"

# ---------------------------------------------------------------------------
# Tools: linuxdeploy + Qt plugin
# ---------------------------------------------------------------------------
mkdir -p "$TOOLS_DIR"

LD="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
LD_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-$ARCH.AppImage"

if [[ ! -x "$LD" ]]; then
    echo "==> Downloading linuxdeploy..."
    curl -fL "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage" -o "$LD"
    chmod +x "$LD"
fi

if [[ ! -x "$LD_QT" ]]; then
    echo "==> Downloading linuxdeploy-plugin-qt..."
    curl -fL "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage" -o "$LD_QT"
    chmod +x "$LD_QT"
fi

# ---------------------------------------------------------------------------
# Icon: convert SVG → PNG (requires rsvg-convert, inkscape, or ImageMagick)
# ---------------------------------------------------------------------------
ICON_SVG="$PROJECT_DIR/resources/icons/app-icon-dark.svg"
ICON_PNG="$SCRIPT_DIR/uplinkirc.png"

if [[ ! -f "$ICON_PNG" ]]; then
    echo "==> Converting icon..."
    if command -v rsvg-convert &>/dev/null; then
        rsvg-convert -w 512 -h 512 -o "$ICON_PNG" "$ICON_SVG"
    elif command -v inkscape &>/dev/null; then
        inkscape --export-width=512 --export-height=512 \
                 --export-filename="$ICON_PNG" "$ICON_SVG"
    elif command -v convert &>/dev/null; then
        convert -background none -resize 512x512 "$ICON_SVG" "$ICON_PNG"
    else
        echo "ERROR: no SVG→PNG converter found."
        echo "       Install one of: librsvg (rsvg-convert), inkscape, or imagemagick"
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
echo "==> Building (Release)..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "$PROJECT_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

# ---------------------------------------------------------------------------
# Install into AppDir
# ---------------------------------------------------------------------------
echo "==> Installing into AppDir..."
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# ---------------------------------------------------------------------------
# Build AppImage
# ---------------------------------------------------------------------------
echo "==> Running linuxdeploy..."

# Point linuxdeploy-plugin-qt at Qt6's qmake
export QMAKE
QMAKE=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || true)
if [[ -z "$QMAKE" ]]; then
    echo "WARNING: qmake not found in PATH — Qt plugin may not bundle libraries correctly"
fi

# Some Qt imageformat plugins (e.g. kimg_jxr from kimageformats) depend on
# libraries not present on the build host (e.g. libjxrglue). Provide stub
# shared libraries for any missing deps so linuxdeploy-plugin-qt can proceed.
# The stubs are compiled into a temp dir added to LD_LIBRARY_PATH.
STUB_DIR=$(mktemp -d)
trap 'rm -rf "$STUB_DIR"' EXIT
QT_PLUGIN_SRC=$(${QMAKE} -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/qt6/plugins")
while IFS= read -r missing; do
    soname="$missing"
    echo "==> Creating stub for missing dep: $soname"
    printf 'void _stub(void){}' | \
        gcc -x c -shared -fPIC -o "$STUB_DIR/$soname" - -Wl,-soname,"$soname"
done < <(find "$QT_PLUGIN_SRC" -name "*.so" -exec ldd {} 2>/dev/null \; \
         | awk '/not found/{print $1}' | sort -u)
if [[ -n "$(ls -A "$STUB_DIR" 2>/dev/null)" ]]; then
    export LD_LIBRARY_PATH="$STUB_DIR:${LD_LIBRARY_PATH:-}"
fi

export OUTPUT="UplinkIRC-$VERSION-$ARCH.AppImage"

# If UPDATE_INFORMATION is set (e.g. by CI), embed zsync metadata for auto-update.
# Format: gh-releases-zsync|owner|repo|latest|UplinkIRC-*-ARCH.AppImage.zsync
if [[ -n "${UPDATE_INFORMATION:-}" ]]; then
    export UPDATE_INFORMATION
    echo "==> Embedding update info: $UPDATE_INFORMATION"
fi

# linuxdeploy's bundled strip can't handle modern .relr.dyn sections; shadow it with a no-op
STRIP_WRAP=$(mktemp -d)
printf '#!/bin/sh\nexit 0\n' > "$STRIP_WRAP/strip"
chmod +x "$STRIP_WRAP/strip"
export PATH="$STRIP_WRAP:$PATH"
"$LD" \
    --appdir "$APPDIR" \
    --plugin qt \
    --desktop-file "$SCRIPT_DIR/UplinkIRC.desktop" \
    --icon-file "$ICON_PNG" \
    --output appimage

echo ""
echo "==> Done: $OUTPUT"
