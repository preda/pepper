#include "Lexer.h"
#include "Object.h"
#include "Map.h"
#include "Value.h"
#include "SymbolTable.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

/*
int main(int argc, char **argv) {
    if (argc < 2) { return 1; }
    int len = strlen(argv[1]);
    int i = 10000000;
    u64 h = 0;
    while (i--) {
        h = hash64((byte *) argv[1], len, h);
    }
    printf("%llx\n", h);
}
*/

long long foobar1(long long a, int b) {
    return a + b;
}

union V {
    long long ll;
    double d;
};

V foobar2(V a, int b) {
    V r;
    r.ll = a.ll + b;
    return r;
}

void f(int a) {
    int b;
    printf("f: %p %p\n", &a, &b);
}

void ff() {
    int a;
    void *p = alloca(4);
    printf("ff: %p %p\n", &a, p);
    f(a);
}

typedef int (*F)(...);

int main(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }
    ff();
    void *dl = dlopen(0, 0);
    void *p  = dlsym(dl, "printf");
    printf("%p %p\n", dl, p);
    
    F f = (F) p;
    f("%s - %d\n", "foo", 10);

    Map *map = Map::alloc(4);
    

    Value v = VAL_DOUBLE(1);
    printf("%ld %x %f\n", sizeof(v), (int)TAG(v), getDouble(v));
    v = VAL_DOUBLE(.0/.0);
    printf("%ld %x %f\n", sizeof(v), (int)TAG(v), getDouble(v));
    v = VAL_DOUBLE(0);
    printf("%ld %x %f\n", sizeof(v), (int)TAG(v), getDouble(v));

    int SIZE = 64 * 1024;
    char *buf = new char[SIZE];
    FILE *fi = fopen(argv[1], "r");
    if (!fi) {
        return 2;
    }
    int size = fread(buf, 1, SIZE, fi);
    fclose(fi);
    if (size >= SIZE) {
        return 3;
    }
    buf[size] = 0;
    Lexer lexer(buf);
    TokenInfo info;
    while (true) {
        info.string = "";
        info.number = 0;
        int token = lexer.nextToken(&info);
        if (!token) {
            break;
        }

        if (token == TK_NUMBER) {
            printf("%f ", info.number);
        } else if (token == TK_NAME) {
            printf("%s ", info.string);
        } else if (token == TK_STRING) {
            printf("\"%s\"", info.string);
        } else if (token >= TK_BEGIN_KEYWORD && token < TK_END_KEYWORD) {
            printf("<%s> ", tokens[token]);
        } else if (token < 256) {
            printf("%c ", (char) token);
        } else {
            printf("%c= ", (char) (token - TK_EQUAL));
        }
    }
}
