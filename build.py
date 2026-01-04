#!/usr/bin/env python3
import os
import subprocess
import sys
import platform

def run_command(command, cwd=None):
    print(f"Running: {' '.join(command)}")
    result = subprocess.run(command, cwd=cwd)
    return result.returncode == 0

def main():
    source_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(source_dir, "build")

    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # 1. Try Vcpkg Configuration
    vcpkg_path = os.path.join(source_dir, "vcpkg", "vcpkg")
    if platform.system() == "Windows":
        vcpkg_path += ".exe"

    vcpkg_toolchain = os.path.join(source_dir, "vcpkg", "scripts", "buildsystems", "vcpkg.cmake")
    has_vcpkg = os.path.exists(vcpkg_path) and os.path.exists(vcpkg_toolchain)

    cmake_args = ["cmake", "-S", source_dir, "-B", build_dir]
    cmake_args.extend(["-DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE"])  # Force verbose CMake output
    
    # Pass LUA_DIR if set in environment (for CI)
    if "LUA_DIR" in os.environ:
        lua_dir = os.environ["LUA_DIR"]
        cmake_args.extend([f"-DLUA_DIR={lua_dir}"])
        print(f"Using LUA_DIR={lua_dir}")

    if has_vcpkg:
        print("--- Vcpkg detected, attempting to use it ---")
        # Attempt to install deps first
        vcpkg_install_cmd = [vcpkg_path, "install"]
        if platform.system() == "Windows":
             vcpkg_install_cmd.extend(["--triplet", "x64-windows"])

        if run_command(vcpkg_install_cmd, cwd=source_dir):
            cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain}")
        else:
            print("--- Vcpkg install failed. Falling back to FetchContent (CMake internal) ---")
    else:
        print("--- Vcpkg not found. Using FetchContent (CMake internal) for dependencies ---")

    # 2. Configure CMake
    print("--- Configuring CMake ---")
    if not run_command(cmake_args):
        print("CMake configuration failed.")
        sys.exit(1)

    # 3. Build
    print("--- Building project ---")
    build_cmd = ["cmake", "--build", build_dir, "-j"]
    if platform.system() != "Windows":
        # Force verbose Make output to see actual compiler commands and errors
        build_cmd.extend(["--", "VERBOSE=1", "CXXFLAGS=-v"])
    if not run_command(build_cmd):
        print("Build failed.")
        sys.exit(1)

    print("Build successful!")

if __name__ == "__main__":
    main()