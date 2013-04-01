// Copyright (C) 2013 Mihai Preda

#pragma once

#include "common.h"

class Stack {
 public:
    Value *base;
    unsigned size;
    // unsigned top;

    Stack();
    ~Stack();

    Value *maybeGrow(Value *regs, unsigned space);
    void shrink();
};
