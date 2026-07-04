[settings]
arch=armv8
build_type=Release
compiler=clang
compiler.cppstd=23
compiler.libcxx=libc++
compiler.version=22
os=Macos

[conf]
tools.build:compiler_executables={"c": "/opt/homebrew/opt/llvm/bin/clang", "cpp": "/opt/homebrew/opt/llvm/bin/clang++"}
