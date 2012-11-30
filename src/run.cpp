#include "Parser.h"
#include "VM.h"
#include "Decompile.h"

#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>

void printFunc(Func *);

/*
unsigned foofoo1(double d) {
    return (unsigned) d;
}

double foofoo2(unsigned a) {
    return (double) a;
}

double foofoo3(long long a) {
    return (double) a;
}
*/

int main(int argc, char **argv) {
    if (argc < 2) {
        return 1;
        /*
        foofoo1(1);
        foofoo2(2);
        foofoo3(3);
        */
    }
    FILE *fi = fopen(argv[1], "rb");
    if (!fi) {
        return 2;
    }
    fseek(fi, 0, SEEK_END);
    size_t size = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    
    const char *text = (const char *) mmap(0, size, PROT_READ, MAP_SHARED, fileno(fi), 0);
    fclose(fi);

    timeval tv;
    timezone tz;
    gettimeofday(&tv, &tz);
    long long t1 = tv.tv_sec * 1000000 + tv.tv_usec;
    Func *f = Parser::parseStatList(text);
    if (argc == 3) {
        printFunc(f);
    }

    gettimeofday(&tv, &tz);
    long long t2 = tv.tv_sec * 1000000 + tv.tv_usec;
    if (!f) {
        return 3;
    }
    Value ret = VM().run(f);
    gettimeofday(&tv, &tz);
    long long t3 = tv.tv_sec * 1000000 + tv.tv_usec;
    printf("compilation %lld execution %lld\n", (t2-t1), (t3-t2));
    printValue(ret);
}
