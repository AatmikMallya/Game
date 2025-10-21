#!/bin/bash
#
# Voxel Engine Automated Benchmark Launcher
# Compiles the extension (if needed) and runs performance tests
#

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/project"
GODOT_BIN="${GODOT_BIN:-/Applications/Godot.app/Contents/MacOS/Godot}"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Voxel Engine Benchmark System${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Parse arguments
SKIP_BUILD=false
REBUILD=false
PLATFORM="macos"
TARGET="template_debug"
JOBS=9

while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --rebuild)
            REBUILD=true
            shift
            ;;
        --release)
            TARGET="template_release"
            shift
            ;;
        --platform=*)
            PLATFORM="${1#*=}"
            shift
            ;;
        --jobs=*)
            JOBS="${1#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --skip-build       Skip compilation, run benchmark only"
            echo "  --rebuild          Clean and rebuild extension"
            echo "  --release          Use release build instead of debug"
            echo "  --platform=NAME    Target platform (default: macos)"
            echo "  --jobs=N           Parallel compilation jobs (default: 9)"
            echo "  --help             Show this help message"
            echo ""
            echo "Environment Variables:"
            echo "  GODOT_BIN          Path to Godot executable"
            echo "                     (default: /Applications/Godot.app/Contents/MacOS/Godot)"
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Warning: Unknown option $1${NC}"
            shift
            ;;
    esac
done

# Check if Godot exists
if [ ! -f "$GODOT_BIN" ]; then
    echo -e "${YELLOW}Error: Godot not found at: $GODOT_BIN${NC}"
    echo "Set GODOT_BIN environment variable or use default path"
    exit 1
fi

# Build extension
if [ "$SKIP_BUILD" = false ]; then
    echo -e "${GREEN}[1/2] Building Extension...${NC}"
    echo "Platform: $PLATFORM | Target: $TARGET | Jobs: $JOBS"
    echo ""

    if [ "$REBUILD" = true ]; then
        echo "Cleaning build artifacts..."
        scons -c platform=$PLATFORM target=$TARGET 2>&1 || true
    fi

    scons platform=$PLATFORM target=$TARGET -j$JOBS

    if [ $? -ne 0 ]; then
        echo -e "${YELLOW}Error: Build failed${NC}"
        exit 1
    fi
    echo -e "${GREEN}Build complete!${NC}"
    echo ""
else
    echo -e "${BLUE}[1/2] Skipping build (--skip-build)${NC}"
    echo ""
fi

# Run benchmark
echo -e "${GREEN}[2/2] Running Benchmark...${NC}"
echo "Scene: res://benchmark/benchmark.tscn"
echo ""

# Create results directory if it doesn't exist
mkdir -p "$PROJECT_DIR/.godot/benchmark_results"

# Launch Godot with benchmark scene
# Note: We don't use --quit-after because the benchmark controller handles quitting
"$GODOT_BIN" \
    --path "$PROJECT_DIR" \
    --scene res://benchmark/benchmark.tscn

# Exit with Godot's exit code
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Benchmark Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Results saved to:"
    echo "  ~/Library/Application Support/Godot/app_userdata/game/benchmark_results/"
    echo ""
else
    echo -e "${YELLOW}Benchmark exited with code: $EXIT_CODE${NC}"
fi

exit $EXIT_CODE
