// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "common.h"

class VM;

Value builtinFileRead(VM *vm, int op, void *data, Value *stack, int nCallArgs);
Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs);
Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs);
Value builtinParseFunc(VM *vm, int op, void *data, Value *stack, int nCallArg);
Value builtinParseBlock(VM *vm, int op, void *data, Value *stack, int nCallArg);
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg);
