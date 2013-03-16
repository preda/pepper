// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "common.h"

class VM;

Value builtinGC(VM *vm, int op, void *data, Value *stack, int nCallArg);
Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs);
Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs);
Value builtinImport(VM *vm, int op, void *data, Value *stack, int nCallArg);
Value androidBackground(VM *vm, int op, void *data, Value *stack, int nCallArg);
