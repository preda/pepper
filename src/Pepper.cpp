// Copyright (C) 2012 - 2013 Mihai Preda

#include "Pepper.h"
#include "GC.h"
#include "Parser.h"
#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "SymbolTable.h"
#include "String.h"
#include <assert.h>

Pepper::Pepper(void *context) :
    _gc(new GC()),
    vm(new VM(this)),
    _syms(SymbolTable::alloc(_gc))
{
    assert(sizeof(Array) == 2 * sizeof(long));
    _syms->set(String::value(_gc, "builtin"), 0);
    _gc->addRoot((Object *) _syms);
}

Pepper::~Pepper() {
    delete _gc;
    _gc = 0;
    delete vm;
    vm = 0;
    _syms = 0; // garbage collected
}

Value Pepper::run(Func *f, int nArg, Value *args) {
    return vm->run(f, nArg, args);
}

Func *Pepper::parseFunc(const char *text) {
    return Parser::parseInEnv(this, text, true);
}

Func *Pepper::parseStatList(const char *text) {
    return Parser::parseInEnv(this, text, false);
}
