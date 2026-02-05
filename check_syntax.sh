#!/usr/bin/env bash
# Quick syntax check for a single C++ file without full linking
# Usage: ./check_syntax.sh src/game/world_context/context_manager.cpp

set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <source_file.cpp>"
    echo "Example: $0 src/game/world_context/context_manager.cpp"
    exit 1
fi

SOURCE_FILE="$1"
PRESET="${2:-linux-debug}"
BUILD_DIR="build/${PRESET}"

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Error: File not found: $SOURCE_FILE" >&2
    exit 1
fi

# Ensure debug build is configured (required for single-file compilation)
if [[ ! -f "${BUILD_DIR}/build.ninja" ]]; then
    echo "Debug build not configured. Running initial configuration..."
    echo "This is a one-time setup that takes ~2 minutes."
    echo ""
    VCPKG_ROOT="${VCPKG_ROOT:-$HOME/repos/vcpkg}" cmake --preset "$PRESET"
fi

# Determine library name based on preset
if [[ "$PRESET" == *"debug"* ]]; then
    LIB_NAME="canary-debug_lib"
else
    LIB_NAME="canary_lib"
fi

# Convert source path to object target
# e.g., src/game/world_context/context_manager.cpp 
#    -> src/CMakeFiles/canary-debug_lib.dir/game/world_context/context_manager.cpp.o
RELATIVE_PATH="${SOURCE_FILE#src/}"
OBJECT_TARGET="src/CMakeFiles/${LIB_NAME}.dir/${RELATIVE_PATH}.o"

echo "=== Compiling: $SOURCE_FILE ==="
echo ""

cd "$BUILD_DIR"

# Compile just this one file
if ninja "$OBJECT_TARGET" 2>&1; then
    echo ""
    echo "✓ Syntax OK: $SOURCE_FILE"
else
    echo ""
    echo "✗ Compilation failed: $SOURCE_FILE"
    exit 1
fi
