#!/usr/bin/env python3
import os
import subprocess
import sys
import platform
import shutil

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
    vcpkg_candidates = []
    if "VCPKG_ROOT" in os.environ:
        vcpkg_candidates.append(os.environ["VCPKG_ROOT"])
    vcpkg_candidates.append(os.path.join(source_dir, "vcpkg"))

    vcpkg_root = None
    vcpkg_path = None
    vcpkg_toolchain = None

    for candidate in vcpkg_candidates:
        candidate_exe = os.path.join(candidate, "vcpkg")
        if platform.system() == "Windows":
            candidate_exe += ".exe"
        
        candidate_toolchain = os.path.join(candidate, "scripts", "buildsystems", "vcpkg.cmake")

        if os.path.exists(candidate_exe) and os.path.exists(candidate_toolchain):
            vcpkg_root = candidate
            vcpkg_path = candidate_exe
            vcpkg_toolchain = candidate_toolchain
            break
    
    has_vcpkg = vcpkg_root is not None

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
    
    # 4. Update Assets
    print("--- Updating Assets ---")
    assets_src = os.path.join(source_dir, "assets")
    assets_dst = os.path.join(build_dir, "assets")
    
    # Remove old assets in build folder to ensure clean state
    if os.path.exists(assets_dst):
        shutil.rmtree(assets_dst)
    
    # Copy fresh assets
    shutil.copytree(assets_src, assets_dst)
    print(f"Assets copied to {assets_dst}")

    print("Build successful!")

if __name__ == "__main__":
    main()