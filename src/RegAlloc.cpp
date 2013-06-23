// Copyright (C) 2013 Mihai Preda

#include "RegAlloc.h"
#include <assert.h>
#include <algorithm>

bool RegAlloc::Reg::add(Var &var) {
    for (Var v : vars) {
        if (var.overlaps(v)) {
            return false;
        }
    }
    vars.push_back(var);
    return true;
}

void RegAlloc::write(unsigned v, int pos) {
    if (v == vars.size()) {
        vars.push_back(Var(v, pos));
    } else {
        assert(0 <= v && v < vars.size() && vars[v].begin < pos);        
    }    
}

void RegAlloc::read(unsigned v, int pos) {
    assert(0 <= v && v < vars.size() && vars[v].end < pos);
    vars[v].end = pos;
}

void RegAlloc::alloc(Var &var) {
    // if (allocReg[var.name] >= 0) { return; }
    for (auto it = regs.begin(), end = regs.end(); it < end; ++it) {
        if (it->add(var)) {
            allocReg[var.name] = it - regs.begin();
            return;
        }
    }
    Reg reg;
    bool ok = reg.add(var);
    assert(ok);
    regs.push_back(reg);
    allocReg[var.name] = regs.size() - 1;
}

int RegAlloc::doAllocation(int nArg, int *args) {
    allocReg.resize(vars.size());
    std::fill(allocReg.begin(), allocReg.end(), -1);
    for (int i = 0; i < nArg; ++i) {
        alloc(vars[args[i]]);
    }    
    std::sort(vars.begin(), vars.end());
    for (Var &var : vars) {
        if (allocReg[var.name] == -1) {
            alloc(var);
        }
    }
    return regs.size();
}
