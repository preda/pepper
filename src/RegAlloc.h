// Copyright (C) 2013 Mihai Preda

#include <vector>

class SpanTracker {
 public:
    void read(unsigned var, int pos);
    void write(unsigned var, int pos);
    std::vector<int> varToReg(int nArgs, int *args);

    struct Var {
        Var(int name, int begin) : name(name), begin(begin), end(begin) {}

        int span() const { return end - begin; }

        bool overlaps(Var other) {
            return !(end <= other.begin || other.end <= begin);
        }
        
        bool operator<(const Var &other) const {
            int s1 = span();
            int s2 = other.span();
            return s1 > s2 || (s1 == s2 && begin < other.begin);
        }

        unsigned name;
        int begin;
        int end;
    };

 private:
    std::vector<Var> vars;
};
