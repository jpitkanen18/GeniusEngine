#!/bin/bash
# Downloads and sets up third-party dependencies (GLAD + ImGui)

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/third_party"

echo "=== Setting up third-party dependencies ==="

# --- GLAD ---
if [ ! -f "glad/src/glad.c" ]; then
    echo "[GLAD] Downloading OpenGL 4.1 Core loader..."
    mkdir -p glad/src glad/include/glad glad/include/KHR

    # Download glad (pre-generated for OpenGL 4.1 Core)
    curl -sL "https://raw.githubusercontent.com/nicebyte/nicegraf/master/third_party/glad/src/glad.c" -o /dev/null 2>/dev/null || true

    # We'll generate it inline since glad web service may not be available
    echo "  [INFO] Please generate GLAD files from https://glad.dav1d.de/"
    echo "         Settings: Language=C, Specification=OpenGL, Profile=Core, API gl=4.1"
    echo "         Place files in: third_party/glad/"
    echo ""
    echo "  Or use the glad2 pip package:"
    echo "    pip install glad2"
    echo "    glad --api gl:core=4.1 --out-path third_party/glad c"

    # Check if python glad is available
    if command -v glad &> /dev/null; then
        echo "  [GLAD] Found glad CLI, generating..."
        glad --api gl:core=4.1 --out-path glad c
        echo "  [OK] GLAD generated"
    elif command -v python3 &> /dev/null && python3 -c "import glad" 2>/dev/null; then
        echo "  [GLAD] Found glad Python module, generating..."
        python3 -m glad --api gl:core=4.1 --out-path glad c
        echo "  [OK] GLAD generated"
    else
        echo "  [WARN] Could not auto-generate GLAD. Install with: pip3 install glad2"
        echo "         Then re-run this script, or download manually."
    fi
else
    echo "[GLAD] Already present"
fi

# --- ImGui ---
if [ ! -f "imgui/imgui.cpp" ]; then
    echo "[ImGui] Downloading Dear ImGui..."

    IMGUI_VERSION="v1.90.4"
    IMGUI_URL="https://github.com/ocornut/imgui/archive/refs/tags/${IMGUI_VERSION}.tar.gz"

    curl -sL "$IMGUI_URL" -o imgui.tar.gz
    tar xzf imgui.tar.gz
    mv imgui-*/ imgui/
    rm imgui.tar.gz
    echo "  [OK] ImGui ${IMGUI_VERSION} downloaded"
else
    echo "[ImGui] Already present"
fi

echo ""
echo "=== Third-party setup complete ==="
echo "Now run: ./configure.sh && make"
