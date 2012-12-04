#include "Parser.h"
#include "VM.h"
#include "Decompile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>

void printFunc(Func *);

int main(int argc, char **argv) {
    // printf("%u\n", (unsigned)GET_NUM(VAL_NUM(-1)));

    if (argc < 2) { return 1; }
    bool verbose = false;
    if (!strcmp(argv[1], "-v")) {
        verbose = true;
        --argc;
        ++argv;
    }
    if (argc < 2) { return 1; }

    FILE *fi = fopen(argv[1], "rb");
    if (!fi) { return 2; }

    fseek(fi, 0, SEEK_END);
    size_t size = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    
    const char *text = (const char *) mmap(0, size, PROT_READ, MAP_SHARED, fileno(fi), 0);
    fclose(fi);

    timeval tv;
    timezone tz;
    gettimeofday(&tv, &tz);
    long long t1 = tv.tv_sec * 1000000 + tv.tv_usec;
    Func *f = Parser::parseFunc(text);
    if (verbose) {
        printFunc(f);
    }

    gettimeofday(&tv, &tz);
    long long t2 = tv.tv_sec * 1000000 + tv.tv_usec;
    if (!f) {
        return 3;
    }
    Value fargs[5];
    int n = argc - 2;
    if (n > 5) { n = 5; }
    for (int i = 0; i < n; ++i) {
        fargs[i] = VAL_NUM(atoi(argv[2 + i]));                     
    }
    Value ret = VM().run(f, n, fargs);
    gettimeofday(&tv, &tz);
    long long t3 = tv.tv_sec * 1000000 + tv.tv_usec;
    printf("compilation %lld execution %lld\n", (t2-t1), (t3-t2));
    printValue(ret);
}
