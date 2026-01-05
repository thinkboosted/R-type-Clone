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
    vcpkg_root = os.environ.get("VCPKG_ROOT", os.path.join(source_dir, "vcpkg"))
    vcpkg_path = os.path.join(vcpkg_root, "vcpkg")
    if platform.system() == "Windows":
        vcpkg_path += ".exe"

    vcpkg_toolchain = os.path.join(vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake")
    has_vcpkg = os.path.exists(vcpkg_path) and os.path.exists(vcpkg_toolchain)

    cmake_args = ["cmake", "-S", source_dir, "-B", build_dir]

    if has_vcpkg:
        print(f"--- Vcpkg detected at {vcpkg_root}, attempting to use it ---")
        # Attempt to install deps first
        vcpkg_install_cmd = [vcpkg_path, "install"]
        if platform.system() == "Windows":
            vcpkg_install_cmd.extend(["--triplet", "x64-windows"])

        if run_command(vcpkg_install_cmd, cwd=source_dir):
            cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain}")
        else:
            print("--- Vcpkg install failed. Exiting. ---")
            sys.exit(1)
    else:
        print("--- Vcpkg not found. Using FetchContent (CMake internal) if configured ---")

    # 2. Configure CMake
    print("--- Configuring CMake ---")
    if not run_command(cmake_args):
        print("CMake configuration failed.")
        sys.exit(1)

    # 3. Build
    print("--- Building project ---")
    build_cmd = ["cmake", "--build", build_dir, "-j"]

    if not run_command(build_cmd):
        print("Build failed.")
        sys.exit(1)

    print("Build successful!")

if __name__ == "__main__":
    main()