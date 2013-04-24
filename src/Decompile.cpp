// Copyright (C) 2012 - 2013 Mihai Preda

#include "Decompile.h"
#include "Value.h"
#include "VM.h"
#include "Proto.h"
#include "Object.h"
#include "String.h"
#include "StringBuilder.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define _(n) #n
static const char *opNames[] = {
    #include "opcodes.inc"
};
#undef _

void printValue(char *buf, int bufSize, Value a) {
    StringBuilder sb;
    sb.append(a);
    snprintf(buf, bufSize, "%s\n", sb.cstr());
}

void printValue(Value a) {
    char buf[256];
    printValue(buf, sizeof(buf), a);
    fprintf(stderr, "%s\n", buf);
}

static void printOperand(char *buf, int bufSize, int v) {
    snprintf(buf, bufSize, "%d", v);
}

void printBytecode(unsigned *start, int size) {
    // static bool inited = init();
    // (void) inited;
    assert(sizeof(opNames) / sizeof(opNames[0]) == N_OPCODES);
    char sa[32], sb[32], sc[32];
    char buf[256];
    for (unsigned *p = start, *end = start + size; p < end; ++p) {
        unsigned code = *p;
        int i = p - start;
        int op = OP(code), a = OA(code), b = OB(code), c = OC(code);
        printOperand(sa, sizeof(sa), a);
        printOperand(sb, sizeof(sb), b);
        printOperand(sc, sizeof(sc), c);
        printf("%2d: %02x%02x%02x%02x   %-6s ", i, op, c, a, b, opNames[op]);
        switch (op) {
        case JMP:    printf("%3d\n", i + OD(code) + 1); break;

        case JF: 
        case JT:
        case FOR:
        case LOOP:
            printf("%3d,  %3d\n", i + OD(code) + 1, c); break;

        case JLT:
        case JNIS: printf("%3d,  %3d %3d\n", i + OSC(code) + 1, a, b); break;

        case RET:    printf("%3d\n", a); break;

        case MOVE_I: printf("%3d,  #%d\n", c, (int)OD(code)); break;

        case MOVE_C: {
            Value v = *(p+1) | ((u64)*(p+2) << 32);
            printValue(buf, sizeof(buf), v);
            printf("%3d, #%s\n", c, buf);            
            p += 2;
            
            if (IS_PROTO(v)) {
                printf("\n");
                printProto((Proto *) GET_OBJ(v));
                printf("\n");
            }

            break;
        }
            
        case MOVE_R:
        case LEN:
        case FUNC:
        case NOTL:
            printf("%3s,  %3s\n", sc, sa);
            break;

        case GETI:
        case GETF:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
            break;

        case SETI:
        case SETF:
            printf("%3s %3s,  %3s\n", sc, sa, sb);
            break;
            
        default:
            printf("%3s,  %3s %3s\n", sc, sa, sb);
        }
    }
}

void printProto(Proto *proto) {
    /*
    printf("UpVals: ");
    int i = 0;
    // char buf[64];
    for (short *p = proto->ups.buf+i, *end = p-i + proto->ups.size; p < end; ++p, ++i) {
        printf("%3d, ", (int)*p);
    }
    printf("\n\nCode:\n");
    */
    printBytecode(proto->code.buf(), proto->code.size());
}

void printFunc(Func *func) {
    printProto(func->proto);
}
