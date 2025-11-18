#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Release"
TARGET="native"
DO_CONFIGURE=true
DO_BUILD=true
DO_CLEAN=false
SERVE=false
PORT=8080
EMSDK_ENV=""
JOBS=""

info() {
    printf '\033[1;34m[build]\033[0m %s\n' "$*"
}

warn() {
    printf '\033[1;33m[warn]\033[0m %s\n' "$*" >&2
}

die() {
    printf '\033[1;31m[error]\033[0m %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage: ./build.sh [options]

Targets:
  --native                Configure+build the desktop OpenGL target (default)
  --web                   Configure+build the Emscripten/WebAssembly target

Configuration:
  --debug | --release     Shortcut for -DCMAKE_BUILD_TYPE (defaults to Release)
  --build-type <type>     Explicit CMake build type (e.g. RelWithDebInfo)
  --emsdk <path>          Path to emsdk root or emsdk_env.sh (web builds)
  -j, --jobs <n>          Parallel build jobs (falls back to CPU count)
  --configure-only        Stop after the CMake configure step
  --build-only            Skip configure and only run cmake --build
  --clean                 Remove the target build directory before building
  --clean-only            Remove the build directory and exit
  --serve [--port N]      After a web build, launch python -m http.server
  -h, --help              Show this help

Examples:
  ./build.sh --native --debug
  ./build.sh --web --emsdk ~/emsdk/emsdk_env.sh -j 8
  ./build.sh --web --serve --port 9000
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --native)
                TARGET="native"
                ;;
            --web)
                TARGET="web"
                ;;
            --debug)
                BUILD_TYPE="Debug"
                ;;
            --release)
                BUILD_TYPE="Release"
                ;;
            --build-type)
                [[ $# -ge 2 ]] || die "--build-type requires a value"
                BUILD_TYPE="$2"
                shift
                ;;
            -j|--jobs)
                [[ $# -ge 2 ]] || die "--jobs requires a number"
                JOBS="$2"
                shift
                ;;
            --configure-only)
                DO_BUILD=false
                ;;
            --build-only)
                DO_CONFIGURE=false
                ;;
            --clean)
                DO_CLEAN=true
                ;;
            --clean-only)
                DO_CLEAN=true
                DO_CONFIGURE=false
                DO_BUILD=false
                ;;
            --emsdk)
                [[ $# -ge 2 ]] || die "--emsdk requires a path"
                EMSDK_ENV="$2"
                shift
                ;;
            --serve)
                SERVE=true
                ;;
            --port)
                [[ $# -ge 2 ]] || die "--port requires a number"
                PORT="$2"
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "Unknown option: $1"
                ;;
        esac
        shift
    done
}

resolve_emsdk_env_script() {
    local candidate

    if [[ -n "$EMSDK_ENV" ]]; then
        if [[ -d "$EMSDK_ENV" ]]; then
            candidate="$EMSDK_ENV/emsdk_env.sh"
        else
            candidate="$EMSDK_ENV"
        fi
        [[ -f "$candidate" ]] || die "Could not find emsdk_env.sh at $candidate"
        printf '%s' "$candidate"
        return 0
    fi

    if [[ -n "${EMSDK:-}" && -f "${EMSDK}/emsdk_env.sh" ]]; then
        printf '%s' "${EMSDK}/emsdk_env.sh"
        return 0
    fi
    if [[ -n "${EMSDK_HOME:-}" && -f "${EMSDK_HOME}/emsdk_env.sh" ]]; then
        printf '%s' "${EMSDK_HOME}/emsdk_env.sh"
        return 0
    fi
    if [[ -n "${EMSDK_PATH:-}" && -f "${EMSDK_PATH}/emsdk_env.sh" ]]; then
        printf '%s' "${EMSDK_PATH}/emsdk_env.sh"
        return 0
    fi

    printf ''
}

jobs_value() {
    if [[ -n "$JOBS" ]]; then
        printf '%s' "$JOBS"
        return 0
    fi

    if command -v nproc >/dev/null 2>&1; then
        nproc
        return 0
    fi
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu
        return 0
    fi

    printf ''
}

cmake_supports_parallel() {
    local version major minor patch
    version=$(cmake --version | head -n1 | awk '{print $3}') || return 1
    IFS='.' read -r major minor patch <<<"$version"
    if (( major > 3 )); then
        return 0
    fi
    if (( major == 3 && minor >= 12 )); then
        return 0
    fi
    return 1
}

configure_native() {
    info "Configuring native target in $BUILD_DIR"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
}

configure_web() {
    local env_script
    env_script=$(resolve_emsdk_env_script || true)
    if [[ -n "$env_script" ]]; then
        info "Sourcing EMSDK environment: $env_script"
        # shellcheck disable=SC1090
        source "$env_script"
    fi
    command -v emcmake >/dev/null 2>&1 || die "emcmake not found. Run 'source /path/to/emsdk_env.sh' or pass --emsdk <path>."
    info "Configuring web target in $BUILD_DIR"
    emcmake cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
}

build_target() {
    [[ -d "$BUILD_DIR" ]] || die "Build directory $BUILD_DIR does not exist. Run configure first."
    local cmd=(cmake --build "$BUILD_DIR")
    local jobs
    jobs=$(jobs_value)
    if [[ -n "$jobs" ]]; then
        if cmake_supports_parallel; then
            cmd+=(--parallel "$jobs")
        else
            warn "CMake is older than 3.12; ignoring parallel build flag."
        fi
    fi
    info "Building target in $BUILD_DIR"
    "${cmd[@]}"
}

serve_web() {
    [[ "$TARGET" == "web" ]] || { warn "--serve is only meaningful for --web builds"; return; }
    [[ -d "$BUILD_DIR" ]] || die "Build directory $BUILD_DIR does not exist."
    command -v python3 >/dev/null 2>&1 || die "python3 not found; cannot serve build output."
    info "Serving $BUILD_DIR via http://localhost:$PORT (Ctrl+C to stop)"
    (cd "$BUILD_DIR" && python3 -m http.server "$PORT")
}

main() {
    parse_args "$@"

    if [[ "$TARGET" == "web" ]]; then
        BUILD_DIR="$ROOT_DIR/build_web"
    else
        BUILD_DIR="$ROOT_DIR/build_native"
        TARGET="native"
    fi

    if $DO_CLEAN; then
        info "Removing $BUILD_DIR"
        rm -rf "$BUILD_DIR"
        if ! $DO_CONFIGURE && ! $DO_BUILD; then
            exit 0
        fi
    fi

    if $DO_CONFIGURE; then
        if [[ "$TARGET" == "web" ]]; then
            configure_web
        else
            configure_native
        fi
    fi

    if $DO_BUILD; then
        build_target
    fi

    if $SERVE; then
        serve_web
    fi
}

main "$@"
