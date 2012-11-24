#!/bin/sh

OPTS="-O2 -std=c++11"

rm -f pepper *.o && \
g++ $OPTS -Wall -c Array.cpp CFunc.cpp Decompile.cpp FFI.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Proto.cpp String.cpp SymbolMap.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp alloc.cpp && \
g++ $OPTS -o pepper -g *.o main.cpp -ldl -lffi && \
g++ $OPTS -o run *.o run.cpp -ldl -lffi && \
g++ $OPTS -o dltest dltest.cpp -ldl
