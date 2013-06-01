// Copyright (C) 2012 - 2013 Mihai Preda

#include "Pepper.h"
#include "GC.h"
#include "Parser.h"
#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "SymbolTable.h"
#include <assert.h>

static void initSymbolTable(GC *gc, SymbolTable *syms) {
}

Pepper::Pepper(void *context) :
    gc(new GC()),
    vm(new VM(gc, context)),
    syms(new SymbolTable())
{
    assert(sizeof(Array) == 2 * sizeof(long));
    initSymbolTable(gc, syms);
}

Pepper::~Pepper() {
    delete gc;
    gc = 0;
    delete vm;
    vm = 0;
    delete syms;
    syms = 0;
}

Value Pepper::run(Func *f, int nArg, Value *args) {
    return vm->run(f, nArg, args);
}

Func *Pepper::parseFunc(const char *text) {
    return Parser::parseInEnv(gc, text, true);
}

Func *Pepper::parseStatList(const char *text) {
    return Parser::parseInEnv(gc, text, false);
}
