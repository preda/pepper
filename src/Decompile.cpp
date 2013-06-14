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
    snprintf(buf, bufSize, "%s", sb.cstr());
}

static const char *operand(char *buf, int bufSize, int v) {
    switch (v) {
    case 255: return "nil";
    case 254: return "#0";
    case 253: return "#1";
    case 252: return "#-1";
    case 251: return "''";
    case 250: return "[]";
    case 249: return "{}";
    // case 248: return "builtin"; -- doesn't have a stable slot
    }
    snprintf(buf, bufSize, "%d", (int)(signed char)v);
    return buf;
}

void printProto(Proto *proto, int indent);

const char *binopName(int op) {
    switch (op) {
    case ADD: return "+";
    case SUB: return "-";
    case MUL: return "*";
    case DIV: return "/";
    case MOD: return "%";
    case POW: return "^";
    case AND: return "&";
    case OR:  return "|";
    case XOR: return "xor";
    case EQ: return "==";
    case NEQ: return "!==";
    case IS: return "is";
    case NIS: return "!is";
    case LT: return "<";
    case LE: return "<=";
    default: return "binop";
    }
}

static void printBytecode(unsigned *start, int size, int indent, bool dump) {
    assert(sizeof(opNames) / sizeof(opNames[0]) == N_OPCODES);
    char bufa[32], bufb[32], bufc[32];
    char buf[256];
    for (unsigned *p = start, *end = start + size; p < end; ++p) {
        unsigned code = *p;
        int i = p - start;
        int op = OP(code), a = OA(code), b = OB(code), c = OC(code);
        const char *sa = operand(bufa, sizeof(bufa), a);
        const char *sb = operand(bufb, sizeof(bufb), b);
        const char *sc = operand(bufc, sizeof(bufc), c);
        if (dump) {
            printf("%*s%02d  %02x%02x%02x%02x ", indent, "", i, op, c, a, b);
        } else {
            printf("%*s%02d  ", indent, "", i);
        }
        switch (op) {
        case JMP:    printf("JMP %d\n", i + OD(code) + 1); break;

        case FOR:
            printf("FOR %d := %d : %d  @ %d\n", c, c+2, c+1, i + OD(code) + 1);
            break;
            
        case JF: 
        case JT:
        case LOOP:
            printf("%s @ %d, %d\n", opNames[op], i + OD(code) + 1, c); break;

        case JLT:
        case JNIS: printf("%s %d, %d %d\n", opNames[op], i + OSC(code) + 1, a, b); break;

        case RET: printf("RET %s\n", a==255 ? "" : sa); break;
        case MOVE_I: printf("%d = #%d\n", c, (int)OD(code)); break;
        case MOVE_C: {
            Value v = *(p+1) | ((u64)*(p+2) << 32);
            printValue(buf, sizeof(buf), v);
            printf("%d = %s\n", c, buf); 
            p += 2;            
            if (IS_PROTO(v)) {
                printProto((Proto *) GET_OBJ(v), indent + 4);
            }
            break;
        }
            
        case MOVE_R: printf("%s = %s\n", sc, sa); break;

        case LEN:  printf("%s = len %s\n", sc, sa); break;
        case FUNC: printf("%s = FUNC(%s)\n", sc, sa); break;
        case NOTL: printf("%s = !%s\n", sc, sa); break;

        case GETI: printf("%s = %s[%s]\n", sc, sa, sb); break;            
        case GETF: printf("%s = %s.%s\n", sc, sa, sb); break;
        case SETI: printf("%s[%s] = %s\n", sc, sa, sb); break;
        case SETF: printf("%s.%3s = %s\n", sc, sa, sb); break;

        case CALL: {
            StringBuilder buf;
            for (int i = 0; i < b; ++i) {
                if (i) { buf.append(", "); }
                buf.append((double)(c + i));
            }
            printf("%s = %s(%s)\n", sc, sa, buf.cstr());
            break;
        }

        default:
            if (op >= ADD) {
                printf("%s = %s %s %s\n", sc, sa, binopName(op), sb);
            } else {
                printf("%s %3s,  %3s %3s\n", opNames[op], sc, sa, sb);
            }
        }
    }
}

void printProto(Proto *proto, int indent) {
    StringBuilder buf;
    int n = proto->ups.size();
    for (int i = 0; i < n; ++i) {
        buf.append((double) proto->ups.get(i));
        buf.append(' ');
    }
    printf("%*sProto %s\n", indent, "", buf.cstr());
    printBytecode((unsigned *)proto->code.buf(), proto->code.size(), indent, true);
}

void printFunc(Func *func) {
    if (func) {
        printProto(func->proto, 0);
    }
}
