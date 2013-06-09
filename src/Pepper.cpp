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
#include "Type.h"
#include "builtin.h"
#include <assert.h>
#include <stdlib.h>

Pepper::Pepper(void *context) :
    _gc(new GC()),
    types(new Types(_gc)),
    vm(new VM(this)),
    _syms(SymbolTable::alloc(_gc)),
    _regs(Array::alloc(_gc))
{
    assert(sizeof(Array) == 2 * sizeof(long));
    _gc->addRoot((Object *) _syms);
    _gc->addRoot((Object *) _regs);
    
    _regs->push(CFunc::value(_gc, builtinParseFunc));  // 0
    _regs->push(CFunc::value(_gc, builtinParseBlock)); // 1
    _regs->push(CFunc::value(_gc, builtinFileRead));   // 2
    _regs->push(CFunc::value(_gc, javaClass));         // 3

    _syms->add(_gc, _regs, "type",   CFunc::value(_gc, builtinType));  // 4
    _syms->add(_gc, _regs, "print",  CFunc::value(_gc, builtinPrint)); // 5
    _syms->add(_gc, _regs, "parse",  VNIL);  // 6
    _syms->add(_gc, _regs, "file",   VNIL);  // 7
    _syms->add(_gc, _regs, "java",   VNIL);  // 8

    Func *f = parseStatList("\
imports := {}\
return fn(name) {\
  if !imports[name] { imports[name] = parse.block(file.read(name + '.pep'))() }\
  return imports[name]\
}");
    
    Value import = run(f, 0, 0);
    _syms->add(_gc, _regs, "import", import);  // 9
}

Pepper::~Pepper() {
    delete vm;
    vm = 0;
    delete types;
    types = 0;
    delete _gc;
    _gc = 0;
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
