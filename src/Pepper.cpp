// Copyright (C) 2012 - 2013 Mihai Preda

#include "Pepper.h"
#include "GC.h"
#include "Parser.h"
#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "SymbolTable.h"
#include "String.h"
#include "CFunc.h"
#include "builtin.h"
#include <assert.h>
#include <stdlib.h>

Pepper::Pepper(void *context) :
    _gc(new GC()),
    vm(new VM(this)),
    _syms(SymbolTable::alloc(_gc)),
    _regs(Array::alloc(_gc))
{
    assert(sizeof(Array) == 2 * sizeof(long));

    _syms->add(_gc, _regs, "type",  CFunc::value(_gc, builtinType));  // 0
    _syms->add(_gc, _regs, "print", CFunc::value(_gc, builtinPrint)); // 1
    _syms->add(_gc, _regs, "parse", VNIL); // 2
    _syms->add(_gc, _regs, "file",  VNIL); // 3
    _syms->add(_gc, _regs, "java",  VNIL); // 4

    _regs->push(CFunc::value(_gc, builtinParseFunc));  // 5
    _regs->push(CFunc::value(_gc, builtinParseBlock)); // 6
    _regs->push(CFunc::value(_gc, builtinFileRead));   // 7
    _regs->push(CFunc::value(_gc, javaClass));         // 8

    _gc->addRoot((Object *) _syms);
    _gc->addRoot((Object *) _regs);
}

Pepper::~Pepper() {
    delete _gc;
    _gc = 0;
    delete vm;
    vm = 0;
    _syms = 0; // garbage collected
}

Value *Pepper::regs() {
    return _regs->vect.buf();
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
