#!/bin/bash
# GeniusEngine run script
# Launches the built binary from the build directory

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BIN="$BUILD_DIR/bin/GeniusEngine"

if [ ! -f "$BIN" ]; then
    echo "[Error] Binary not found at: $BIN"
    echo "Run './configure.sh && make' first."
    exit 1
fi

# Set library path for shared libs
export DYLD_LIBRARY_PATH="$BUILD_DIR/lib:$DYLD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$BUILD_DIR/lib:$LD_LIBRARY_PATH"

# Add Vulkan lib dir if configured
if [ -f "$SCRIPT_DIR/build.config" ]; then
    VK_LIB_DIR=$(grep '^VULKAN_LIB_DIR=' "$SCRIPT_DIR/build.config" | cut -d= -f2-)
    if [ -n "$VK_LIB_DIR" ]; then
        export DYLD_LIBRARY_PATH="$VK_LIB_DIR:$DYLD_LIBRARY_PATH"
        export LD_LIBRARY_PATH="$VK_LIB_DIR:$LD_LIBRARY_PATH"
    fi
    VK_ICD=$(grep '^VULKAN_ICD_FILENAMES=' "$SCRIPT_DIR/build.config" | cut -d= -f2-)
    if [ -n "$VK_ICD" ]; then
        export VK_ICD_FILENAMES="$VK_ICD"
    fi
fi

echo "[GeniusEngine] Launching..."
exec "$BIN" "$@"
