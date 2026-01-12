#!/usr/bin/env bash
# Build and verify GameEngine with unified bin/lib outputs

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"
LIB_DIR="${BUILD_DIR}/lib"
ASSETS_SRC="${ROOT_DIR}/assets"
ASSETS_DST="${BIN_DIR}/assets"

RUN_AFTER_BUILD=false
if [[ "${1-}" == "--run" ]]; then
    RUN_AFTER_BUILD=true
fi

echo "========================================="
echo "  R-Type GameEngine - Build & Verify"
echo "========================================="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo -e "${YELLOW}[1/4] Configuring CMake...${NC}"
# Detect vcpkg toolchain file
VCPKG_TOOLCHAIN=""
if [[ -n "${VCPKG_ROOT-}" && -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]]; then
    VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
elif [[ -f "${ROOT_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" ]]; then
    VCPKG_TOOLCHAIN="${ROOT_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
fi

if [[ -n "${VCPKG_TOOLCHAIN}" ]]; then
    echo "Using vcpkg toolchain: ${VCPKG_TOOLCHAIN}"
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="${VCPKG_TOOLCHAIN}" ..
else
    echo "Warning: vcpkg toolchain not found, proceeding without -DCMAKE_TOOLCHAIN_FILE"
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

echo -e "${YELLOW}[2/4] Building GameEngine static lib...${NC}"
cmake --build . --target GameEngine -j"$(nproc)"

echo -e "${YELLOW}[3/4] Building r-type_client_engine...${NC}"
cmake --build . --target r-type_client_engine -j"$(nproc)"

echo -e "${YELLOW}[4/4] Verifying build artifacts...${NC}"

ARTIFACTS=(
    "${BIN_DIR}/r-type_client_engine"
    "${LIB_DIR}"                  # lib dir must exist
    "${BIN_DIR}"                  # bin dir must exist
)

# Module expectations with aliases (config names vs actual built names)
MODULE_CANDIDATES=(
    "${LIB_DIR}/WindowModule.so|${LIB_DIR}/SFMLWindowManager.so"
    "${LIB_DIR}/RenderModule.so|${LIB_DIR}/GLEWSFMLRenderer.so"
    "${LIB_DIR}/SoundModule.so|${LIB_DIR}/SFMLSoundManager.so"
    "${LIB_DIR}/NetworkModule.so|${LIB_DIR}/NetworkManager.so"
    "${LIB_DIR}/LuaECSManager.so"
    "${LIB_DIR}/ECSSavesManager.so"
    "${LIB_DIR}/BulletPhysicEngine.so"
)

has_any() {
    IFS='|' read -ra paths <<< "$1"
    for p in "${paths[@]}"; do
        [[ -f "$p" ]] && return 0
    done
    return 1
}

ALL_OK=true
for artifact in "${ARTIFACTS[@]}"; do
    if [[ -f "$artifact" || -d "$artifact" ]]; then
        echo -e "${GREEN}  ✓ $artifact${NC}"
    else
        echo -e "${RED}  ✗ $artifact (NOT FOUND)${NC}"
        ALL_OK=false
    fi
done

for group in "${MODULE_CANDIDATES[@]}"; do
    if has_any "$group"; then
        echo -e "${GREEN}  ✓ $(echo "$group" | tr '|' ' / ')${NC}"
    else
        echo -e "${RED}  ✗ $(echo "$group" | tr '|' ' / ') (NOT FOUND)${NC}"
        ALL_OK=false
    fi
done

# Sync assets into bin (symlink preferred, fallback to copy)
echo -e "${YELLOW}Syncing assets to bin/...${NC}"
mkdir -p "${BIN_DIR}"
if [[ -L "${ASSETS_DST}" || -d "${ASSETS_DST}" ]]; then
    rm -rf "${ASSETS_DST}"
fi
if ln -s "${ASSETS_SRC}" "${ASSETS_DST}" 2>/dev/null; then
    echo -e "${GREEN}  ✓ Assets symlinked: ${ASSETS_DST} -> ${ASSETS_SRC}${NC}"
else
    cp -r "${ASSETS_SRC}" "${ASSETS_DST}"
    echo -e "${GREEN}  ✓ Assets copied to ${ASSETS_DST}${NC}"
fi

if [[ "${ALL_OK}" == true ]]; then
    echo -e "\n${GREEN}BUILD SUCCESSFUL${NC}"
else
    echo -e "\n${RED}BUILD INCOMPLETE - Missing artifacts${NC}"
    exit 1
fi

# Optional run
if [[ "${RUN_AFTER_BUILD}" == true ]]; then
    echo -e "${YELLOW}Running r-type_client_engine (local mode)...${NC}"
    pushd "${BIN_DIR}" >/dev/null
    ./r-type_client_engine local
    popd >/dev/null
fi
