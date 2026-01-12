#!/bin/bash
# Build and test GameEngine (Phase 1.1 validation)

set -e  # Exit on error

echo "========================================="
echo "  R-Type GameEngine - Build & Test"
echo "========================================="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Step 1: Configure CMake
echo -e "${YELLOW}[1/4] Configuring CMake...${NC}"
if [ -d "build" ]; then
    cd build
else
    mkdir -p build
    cd build
fi

cmake -DCMAKE_BUILD_TYPE=Release .. || {
    echo -e "${RED}ERROR: CMake configuration failed${NC}"
    exit 1
}

# Step 2: Build GameEngine
echo -e "${YELLOW}[2/4] Building GameEngine core library...${NC}"
cmake --build . --target GameEngine -j$(nproc) || {
    echo -e "${RED}ERROR: GameEngine build failed${NC}"
    exit 1
}

echo -e "${GREEN}✓ GameEngine core library built successfully${NC}"

# Step 3: Build client executable
echo -e "${YELLOW}[3/4] Building r-type_client_engine...${NC}"
cmake --build . --target r-type_client_engine -j$(nproc) || {
    echo -e "${RED}ERROR: Client executable build failed${NC}"
    exit 1
}

echo -e "${GREEN}✓ r-type_client_engine built successfully${NC}"

# Step 4: Verify outputs
echo -e "${YELLOW}[4/4] Verifying build artifacts...${NC}"

ARTIFACTS=(
    "r-type_client_engine"
    "assets/config/client.json"
    "assets/config/server.json"
    "assets/config/local.json"
)

ALL_OK=true
for artifact in "${ARTIFACTS[@]}"; do
    if [ -f "$artifact" ] || [ -d "$artifact" ]; then
        echo -e "${GREEN}  ✓ $artifact${NC}"
    else
        echo -e "${RED}  ✗ $artifact (NOT FOUND)${NC}"
        ALL_OK=false
    fi
done

if [ "$ALL_OK" = true ]; then
    echo ""
    echo -e "${GREEN}========================================="
    echo "  BUILD SUCCESSFUL!"
    echo "=========================================${NC}"
    echo ""
    echo "Run the engine with:"
    echo "  ./r-type_client_engine --help"
    echo "  RTYPE_DEBUG=1 ./r-type_client_engine"
    echo ""
else
    echo -e "${RED}========================================="
    echo "  BUILD INCOMPLETE - Missing artifacts"
    echo "=========================================${NC}"
    exit 1
fi

# Optional: Display binary info
echo "Binary information:"
file r-type_client_engine
ls -lh r-type_client_engine

echo ""
echo "Configuration files:"
ls -lh assets/config/*.json
