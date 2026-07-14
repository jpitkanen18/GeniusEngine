#!/bin/bash
# GeniusEngine configure script
# Detects platform, checks dependencies, generates build config

set -e

echo "=== GeniusEngine Configure ==="
echo ""

# Detect platform
PLATFORM="unknown"
SHARED_EXT=".so"
SHARED_FLAG="-shared"
FRAMEWORKS=""
PKG_CONFIG_DEPS=""

case "$(uname -s)" in
    Darwin*)
        PLATFORM="macos"
        SHARED_EXT=".dylib"
        SHARED_FLAG="-dynamiclib"
        FRAMEWORKS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo"
        echo "[Platform] macOS detected"
        ;;
    Linux*)
        PLATFORM="linux"
        SHARED_EXT=".so"
        SHARED_FLAG="-shared -fPIC"
        echo "[Platform] Linux detected"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLATFORM="windows"
        SHARED_EXT=".dll"
        SHARED_FLAG="-shared"
        echo "[Platform] Windows (MinGW/MSYS) detected"
        ;;
    *)
        echo "[ERROR] Unknown platform: $(uname -s)"
        exit 1
        ;;
esac

# Check for required tools
echo ""
echo "[Checking tools]"

check_tool() {
    if command -v "$1" &> /dev/null; then
        echo "  [OK] $1 found: $(command -v "$1")"
        return 0
    else
        echo "  [MISSING] $1 not found"
        return 1
    fi
}

MISSING=0
check_tool "g++" || check_tool "clang++" || { echo "  [ERROR] No C++ compiler found"; MISSING=1; }
check_tool "pkg-config" || { echo "  [WARN] pkg-config not found, will use manual paths"; }
check_tool "make" || { MISSING=1; }

# Determine compiler
if command -v clang++ &> /dev/null; then
    CXX="clang++"
elif command -v g++ &> /dev/null; then
    CXX="g++"
else
    echo "[ERROR] No C++ compiler found"
    exit 1
fi
echo "  [Compiler] Using: $CXX"

# Check for dependencies
echo ""
echo "[Checking dependencies]"

check_pkg() {
    if command -v pkg-config &> /dev/null && pkg-config --exists "$1" 2>/dev/null; then
        echo "  [OK] $1 ($(pkg-config --modversion "$1"))"
        return 0
    else
        echo "  [MISSING] $1"
        return 1
    fi
}

# Check GLFW
GLFW_OK=0
if check_pkg "glfw3"; then
    GLFW_CFLAGS="$(pkg-config --cflags glfw3)"
    GLFW_LIBS="$(pkg-config --libs glfw3)"
    GLFW_OK=1
elif [ "$PLATFORM" = "macos" ] && [ -d "/opt/homebrew/include/GLFW" ]; then
    echo "  [OK] glfw3 (homebrew manual)"
    GLFW_CFLAGS="-I/opt/homebrew/include"
    GLFW_LIBS="-L/opt/homebrew/lib -lglfw"
    GLFW_OK=1
elif [ "$PLATFORM" = "macos" ] && [ -d "/usr/local/include/GLFW" ]; then
    echo "  [OK] glfw3 (usr/local manual)"
    GLFW_CFLAGS="-I/usr/local/include"
    GLFW_LIBS="-L/usr/local/lib -lglfw"
    GLFW_OK=1
else
    echo "  [MISSING] glfw3 - install with: brew install glfw (macOS) / apt install libglfw3-dev (Linux)"
    MISSING=1
fi

# Check GLM (header-only)
GLM_OK=0
if check_pkg "glm"; then
    GLM_CFLAGS="$(pkg-config --cflags glm)"
    GLM_OK=1
elif [ -d "/opt/homebrew/include/glm" ] || [ -d "/usr/local/include/glm" ] || [ -d "/usr/include/glm" ]; then
    echo "  [OK] glm (found in include path)"
    GLM_CFLAGS=""
    if [ -d "/opt/homebrew/include/glm" ]; then
        GLM_CFLAGS="-I/opt/homebrew/include"
    fi
    GLM_OK=1
else
    echo "  [MISSING] glm - install with: brew install glm (macOS) / apt install libglm-dev (Linux)"
    MISSING=1
fi

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "[ERROR] Missing dependencies. Please install them and re-run configure.sh"
    exit 1
fi

# Check Vulkan (optional)
echo ""
echo "[Checking optional backends]"
HAS_VULKAN=0
VULKAN_CFLAGS=""
VULKAN_LIBS=""
VULKAN_LIB_DIR=""

if [ -f "/usr/local/include/vulkan/vulkan.h" ] && [ -f "/usr/local/lib/libvulkan.dylib" -o -f "/usr/local/lib/libvulkan.so" ]; then
    echo "  [OK] vulkan (usr/local)"
    VULKAN_CFLAGS="-I/usr/local/include"
    VULKAN_LIBS="-L/usr/local/lib -lvulkan"
    VULKAN_LIB_DIR="/usr/local/lib"
    HAS_VULKAN=1
elif check_pkg "vulkan"; then
    VULKAN_CFLAGS="$(pkg-config --cflags vulkan 2>/dev/null)"
    VULKAN_LIBS="$(pkg-config --libs vulkan 2>/dev/null)"
    # Extract lib dir from -L flag
    VULKAN_LIB_DIR="$(echo "$VULKAN_LIBS" | grep -oE '\-L[^ ]+' | head -1 | sed 's/^-L//')"
    HAS_VULKAN=1
elif [ -f "/usr/include/vulkan/vulkan.h" ]; then
    echo "  [OK] vulkan (system)"
    VULKAN_CFLAGS=""
    VULKAN_LIBS="-lvulkan"
    VULKAN_LIB_DIR=""
    HAS_VULKAN=1
elif [ -n "$VULKAN_SDK" ] && [ -f "$VULKAN_SDK/include/vulkan/vulkan.h" ]; then
    echo "  [OK] vulkan (VULKAN_SDK=$VULKAN_SDK)"
    VULKAN_CFLAGS="-I$VULKAN_SDK/include"
    VULKAN_LIBS="-L$VULKAN_SDK/lib -lvulkan"
    VULKAN_LIB_DIR="$VULKAN_SDK/lib"
    HAS_VULKAN=1
else
    echo "  [SKIP] Vulkan SDK not found — Vulkan backend disabled"
    echo "         Install with: brew install vulkan-sdk (macOS) / apt install libvulkan-dev (Linux)"
fi

# Find Vulkan ICD JSON (needed for MoltenVK on macOS)
VULKAN_ICD_FILENAMES=""
if [ "$HAS_VULKAN" -eq 1 ]; then
    for icd_path in \
        "/usr/local/share/vulkan/icd.d/MoltenVK_icd.json" \
        "$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json" \
        "/usr/share/vulkan/icd.d" \
        "/etc/vulkan/icd.d"; do
        if [ -f "$icd_path" ]; then
            VULKAN_ICD_FILENAMES="$icd_path"
            echo "  [OK] Vulkan ICD: $icd_path"
            break
        fi
    done
    if [ -z "$VULKAN_ICD_FILENAMES" ]; then
        echo "  [WARN] Vulkan ICD JSON not found — Vulkan may fail at runtime"
    fi
fi

if [ "$PLATFORM" = "macos" ]; then
    echo "  [OK] Metal (built-in on macOS)"
fi

# Generate build config
echo ""
echo "[Generating build configuration]"

cat > build.config << EOF
# Auto-generated by configure.sh - do not edit manually
PLATFORM=${PLATFORM}
CXX=${CXX}
CXXFLAGS=-std=c++20 -Wall -Wextra -O2 -fPIC
SHARED_EXT=${SHARED_EXT}
SHARED_FLAG=${SHARED_FLAG}
FRAMEWORKS=${FRAMEWORKS}
GLFW_CFLAGS=${GLFW_CFLAGS}
GLFW_LIBS=${GLFW_LIBS}
GLM_CFLAGS=${GLM_CFLAGS}
HAS_VULKAN=${HAS_VULKAN}
VULKAN_CFLAGS=${VULKAN_CFLAGS}
VULKAN_LIBS=${VULKAN_LIBS}
VULKAN_LIB_DIR=${VULKAN_LIB_DIR}
VULKAN_ICD_FILENAMES=${VULKAN_ICD_FILENAMES}
EOF

echo "  [OK] build.config written"
echo ""
BACKENDS="OpenGL"
if [ "$PLATFORM" = "macos" ]; then
    BACKENDS="$BACKENDS, Metal"
fi
if [ "$HAS_VULKAN" -eq 1 ]; then
    BACKENDS="$BACKENDS, Vulkan"
fi
echo "=== Configuration complete ==="
echo "  Backends: $BACKENDS"
echo "  Run 'make' to build GeniusEngine"
