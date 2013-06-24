// Copyright (C) 2013 Mihai Preda

#include "SpanTracker.h"
#include <assert.h>
#include <algorithm>

typedef SpanTracker::Var Var;

void SpanTracker::write(unsigned v, int pos) {
    if (v == vars.size()) {
        vars.push_back(Var(v, pos));
    } else {
        assert(0 <= v && v < vars.size() && vars[v].begin < pos);        
    }    
}

void SpanTracker::read(unsigned v, int pos) {
    assert(0 <= v && v < vars.size() && vars[v].end < pos);
    vars[v].end = pos;
}

class Reg {
    std::vector<Var> vars;
    
public:
    Reg(Var var) { bool ok = add(var); assert(ok); }
    bool add(Var var);
};

bool Reg::add(Var var) {
    for (Var v : vars) {
        if (var.overlaps(v)) {
            return false;
        }
    }
    vars.push_back(var);
    return true;
}

static int alloc(std::vector<Reg> &regs, Var var) {
    for (auto it = regs.begin(), end = regs.end(); it < end; ++it) {
        if (it->add(var)) { return it - regs.begin(); }
    }
    regs.emplace_back(var);
    return regs.size() - 1;
}

std::vector<int> SpanTracker::varToReg(int nArg, int *args) {
    std::vector<Reg> regs;
    std::vector<int> ret(vars.size(), -1);
    for (int i = 0; i < nArg; ++i) {
        Var var = vars[args[i]];
        ret[var.name] = alloc(regs, var);
    }    
    std::sort(vars.begin(), vars.end());
    for (Var &var : vars) {
        const int id = var.name;
        if (ret[id] == -1) {
            ret[id] = alloc(regs, var);
        }
    }
    return ret;
}
