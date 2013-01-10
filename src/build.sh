#!/bin/sh

#OPTS="-O2 -std=c++11"
OPTS="-g -std=c++11"

rm -f run *.o && \
g++ $OPTS -Wall -c builtin.cpp Index.cpp SimpleMap.cpp Pepper.cpp Array.cpp CFunc.cpp Decompile.cpp fnv.cpp Func.cpp GC.cpp Lexer.cpp Map.cpp Object.cpp Parser.cpp Proto.cpp String.cpp SymbolTable.cpp Value.cpp Vector.cpp VM.cpp error.cpp alloc.cpp && \
g++ $OPTS -o pepper -g *.o main.cpp

# -ldl -lffi
