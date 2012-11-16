#!/bin/sh

rm -f pepper *.o && \
g++ -g -Wall -c -std=c++11 Array.cpp CFunc.cpp Decompile.cpp FFI.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Proto.cpp String.cpp SymbolMap.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp main.cpp && \
g++ -o pepper -g *.o -ldl
