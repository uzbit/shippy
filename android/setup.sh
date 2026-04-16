#!/bin/bash
# Downloads SDL3 Java bridge sources needed for Android build.
# Run this once before the first build.

set -e
cd "$(dirname "$0")"

SDL3_VERSION="release-3.4.4"
SDL3_DIR="SDL3-src"

if [ ! -d "$SDL3_DIR" ]; then
    echo "Fetching SDL3 Java sources..."
    git clone --depth 1 --branch "$SDL3_VERSION" \
        https://github.com/libsdl-org/SDL.git "$SDL3_DIR"
    echo "Done."
else
    echo "SDL3 source already present at $SDL3_DIR"
fi
