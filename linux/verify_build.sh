#!/bin/bash

# ====== CXL Framework Compilation Verification Script ======

set -e

echo "========================================="
echo "CXL Framework Compilation Verification"
echo "========================================="
echo ""

# Set colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}[✓] $2${NC}"
    else
        echo -e "${RED}[✗] $2${NC}"
        exit 1
    fi
}

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v gcc &> /dev/null; then
    echo -e "${RED}[ERROR] GCC not found${NC}"
    exit 1
fi
print_status 0 "GCC found: $(gcc --version | head -1)"

if ! pkg-config --exists libnuma; then
    echo -e "${YELLOW}[WARNING] libnuma development library not found${NC}"
    echo "Install with: sudo apt-get install libnuma-dev"
    exit 1
fi
print_status 0 "libnuma found"

echo ""
echo "Building framework..."
echo ""

# Navigate to the linux directory
cd "$(dirname "${BASH_SOURCE[0]}")"

# Clean previous build
echo "Cleaning previous build..."
make clean > /dev/null 2>&1 || true
print_status 0 "Clean completed"

# Build the framework
echo "Compiling framework..."
if make -j$(nproc) > /tmp/build.log 2>&1; then
    print_status 0 "Framework compiled successfully"
else
    print_status 1 "Build failed. See /tmp/build.log for details"
fi

echo ""
echo "Checking build artifacts..."

# Check if binary exists
if [ -f bin/cxl_framework ]; then
    print_status 0 "Binary generated: bin/cxl_framework"
else
    print_status 1 "Binary not found"
fi

# Check file sizes
for file in src/*.c include/*.h; do
    if [ -f "$file" ]; then
        size=$(wc -l < "$file")
        echo -e "${GREEN}[✓]${NC} $file: $size lines"
    fi
done

echo ""
echo "========================================="
echo "Verification Summary"
echo "========================================="

total_files=$(ls src/*.c include/*.h 2>/dev/null | wc -l)
echo -e "${GREEN}Total source files: $total_files${NC}"

total_lines=$(wc -l src/*.c include/*.h 2>/dev/null | tail -1 | awk '{print $1}')
echo -e "${GREEN}Total lines of code: $total_lines${NC}"

echo ""
echo "Framework build verification completed successfully!"
echo ""
echo "Next steps:"
echo "  1. Run the framework: ./bin/cxl_framework"
echo "  2. Check results: ./results/"
echo "  3. View documentation: less README.md"
echo ""
