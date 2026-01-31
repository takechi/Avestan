setlocal
mkdir build_Win32
pushd build_Win32
cmake -G "Visual Studio 18 2026" -A Win32 ..
