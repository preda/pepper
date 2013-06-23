// Copyright (C) 2013 Mihai Preda

#include <vector>

class RegAlloc {
    struct Var {
        Var(int name, int begin) : name(name), begin(begin), end(begin) {}

        int length() const { return end - begin; }

        bool overlaps(Var other) {
            return !(end <= other.begin || other.end <= begin);
        }
        
        bool operator<(const Var &other) const {
            int l1 = length();
            int l2 = other.length();
            return l1 > l2 || (l1 == l2 && begin < other.begin);
        }

        unsigned name;
        int begin;
        int end;
    };

    struct Reg {
        std::vector<Var> vars;
        bool add(Var &var);
    };
    
    std::vector<Var> vars;
    std::vector<Reg> regs;
    std::vector<int> allocReg;
    
    void alloc(Var &var);
    
 public:
    
    void read(unsigned var, int pos);
    void write(unsigned var, int pos);
    int doAllocation(int nArgs, int *args);

    int get(int var);    
};
