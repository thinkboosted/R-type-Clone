$env:VCPKG_DEFAULT_TRIPLET = "x64-mingw-dynamic"
vcpkg install

mkdir ./build/

cmake --preset windows
cmake --build --preset windows

cd ./build/windows/
./Rtype.exe

cd ../../
