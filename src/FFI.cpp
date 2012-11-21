#include "FFI.h"
#include "CFunc.h"
#include "Object.h"
#include "String.h"

#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

#define SIZE(t) (((t) | CPTR) ? sizeof(void*) : typeSize[(t) & 0xf])

const char *nextType(const char *s, const char *delims, int *len, const char **next) {
    while (*s == ' ') { ++s; }
    const char *p = s;
    while (*p && !strchr(delims, *p)) { ++p; }
    const char *p2 = p-1;
    while (*p2 == ' ') { --p2; }
    *len = p2 - s + 1;
    if (*p) { ++p; }
    *next = *p ? p : 0;
    return s;    
}

static int strToType(const char *s, int len) {
    const char *names[] = {
        "void", "const char *", "char *", "void *", 
        "int", "unsigned", "long", "unsigned long", "long long", "unsigned long long",
        "double", "char", "...", "ptrdiff",
    };
    int types[] = {
        CVOID, CCONST|CPTR|CCHAR, CPTR|CCHAR, CPTR|CVOID,
        CINT, CINT, CLONG, CLONG, CLONGLONG, CLONGLONG,
        CDOUBLE, CINT, CELLIPSE, PTRDIFF,
    };
    int n = sizeof(types) / sizeof(types[0]);
    for (int i = 0; i < n; ++i) {
        if (!strncmp(s, names[i], len)) {
            return types[i];
        }
    }
    return -1;
}

int parseTypesC(const char *decl, int maxSize, byte *outTypes, byte *retType, bool *hasEllipsis) {
    *retType = CVOID;
    *hasEllipsis = false;
    const char *next;
    int len;
    const char *s = nextType(decl, "(", &len, &next);
    unsigned t = strToType(s, len);
    if (t < 0) { return 0; }
    *retType = t;
    int n = 0;
    while (next) {
        s = nextType(next, ",)", &len, &next);
        t = strToType(s, len);
        if (t < 0) { return 0; }
        outTypes[n++] = t;
    }
    if (n > 1 && outTypes[n-1]==CELLIPSE) {
        --n;
        *hasEllipsis = true;
    }
    return n;
}

int parseTypesVararg(const char *decl, int maxSize, byte *outTypes) {
    int n = 0;
    const char *p = decl;
    while (n < maxSize) {
        p = strchr(p, '%');
        if (p == 0) { return n; }
        char c;
        while ((c=*p) && !(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))) { ++p; }
    again:
        c = *p;
        if (!c) { return n; }
        int t = 0;
        int lng = 0;
        switch (c) {
        case 's': t = CPTR | CCHAR; break;
        case 'p': t = CPTR; break;
        case 'd': case 'u': case 'c': case 'x':
            t = lng==0 ? CINT : lng==1 ? CLONG : CLONGLONG; 
            break;
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
            t = CDOUBLE;
            break;
        case 'l': ++lng; ++p; goto again;
        }
        outTypes[n++] = t;
    }
    return n;
}

u64 castValue(Value &v, int t) {
    if ((t & CPTR) && (t&0xf)==CCHAR) { // char*
        if (IS_STRING(v)) {
            return (u64) GET_CSTR(v);
        }
    } else if (t & CPTR) {
        // TODO: array -> pointer
    } else if ((t & 0xf) == CDOUBLE) {
        if (IS_NUMBER(v)) {
            u64 tmp;
            *(double*)(&tmp) = getDouble(v);
            return tmp;
        }
    } else if (t==CCHAR || t==CINT || t==CLONG || t==CLONGLONG) {
        if (IS_INT(v)) {
            return getInteger(v);
        }
    }
    return 0;
}

unsigned makeCode(byte *types, int n) {
    static const int typeSize[] = {0, 4, 4, sizeof(long), 8, 8};
    unsigned code = 1;
    for (int i = 0; i < n; ++i) {
        int t = types[i];
        code = (code << 1) | (SIZE(t) >> 3);
    }
    return code;
}

int ffiConstruct(int op, void *data, Value *stack, int nCallArg) {
    Value a = stack[0];
    stack[0] = NIL;
    if (op == 0) {
        if (nCallArg != 2) { return 1; }

        Value b = stack[1];
        if (!IS_STRING(a) || !IS_STRING(b)) {
            return 2;
        }
        const char *fname = GET_CSTR(a);
        const char *signature = GET_CSTR(b);
        void *f = dlsym(0, fname);
        if (!f) { return 1; }
        FFIData d;
        d.f = f;            
        d.nArg = parseTypesC(signature, 8, d.argType, &d.retType, &d.hasEllipsis);
        CFunc *cfunc = CFunc::alloc((tfunc) ffiCall, sizeof(d));
        *(FFIData *)(cfunc->data) = d;
        stack[0] = VAL_OBJ(cfunc);
    }
    return 0;
}

int ffiCall(int op, FFIData *d, Value *stack, int nCallArg) {
    if (op == 0) {
        u64 p1[8] = {0};        
        int nArg = d->nArg;
        byte *argType = d->argType;
        // byte retType = d->retType;
        // unsigned code = 4 | (SIZE(retType) >> 2);
        for (int i = 0; i < nArg; ++i) {
            p1[i] = castValue(stack[i], argType[i]);
        }
        unsigned code = makeCode(argType, nArg);        
        bool hasEllipsis = d->hasEllipsis;
        if (hasEllipsis && nArg >= 1 && argType[nArg-1] == (CCONST|CPTR|CCHAR) && nCallArg >= nArg && IS_STRING(stack[nArg-1])) {
            Value v = stack[nArg-1];
            const char *str = GET_CSTR(v);
            byte elipseTypes[8];
            int n = parseTypesVararg(str, 8, elipseTypes);
            if (nCallArg != nArg + n) {
                return 1;
            }
            for (int i = nArg; i < nCallArg; ++i) {
                p1[i] = castValue(stack[i], elipseTypes[i - nArg]);
            }
            code = (code>>1) | (makeCode(elipseTypes, n) << 4);
        }
        unsigned p0[8] = {0};
        for (int i = 0; i < nCallArg; ++i) {
            p0[i] = (unsigned) p1[i];
        }
        void *f = d->f;

#define T0 unsigned
#define T1 u64
#define F0        case 0b1:          ret = ((u64 (*)()) f)(); break;
#define F1(a)     case 0b1##a:       ret=((u64(*)(T##a))f)(p##a[0]); break;
#define F2(a,b)   case 0b1##a##b:    ret=((u64(*)(T##a,T##b))f)(p##a[0],p##b[1]); break;
#define F3(a,b,c) case 0b1##a##b##c: ret=((u64(*)(T##a,T##b,T##c))f)(p##a[0],p##b[1],p##c[2]); break;
#define FF F0; F1(0); F1(1); F2(0,0); F2(0,1); F2(1,0); F2(1,1); F3(0,0,0); F3(0,0,1); F3(0,1,0); F3(0,1,1); F3(1,0,0); F3(1,0,1); F3(1,1,0); F3(1,1,1);
        u64 ret = 0;
        switch (code) {
            FF;

#undef F0
#undef F1 
#undef F2 
#undef F3

#define F0 \
        case 0b10001: ret=((u64(*)(        char*, ...))f)((char*)p1[0]); break; \
        case 0b10010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1]); break; \
        case 0b10011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1]); break; \
        case 0b10100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2]); break; \
        case 0b10101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2]); break; \
        case 0b10110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2]); break; \
        case 0b10111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2]); break;
            
#define F1(a) \
        case 0b1##a##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1]); break; \
        case 0b1##a##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2]); break; \
        case 0b1##a##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2]); break; \
        case 0b1##a##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3]); break; \
        case 0b1##a##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3]); break;

#define F2(a, b) \
        case 0b1##a##b##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1], p##b[2]); break; \
        case 0b1##a##b##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2], p##b[3]); break; \
        case 0b1##a##b##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2], p##b[3]); break; \
        case 0b1##a##b##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3], p##b[4]); break; \
        case 0b1##a##b##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3], p##b[4]); break;

#define F3(a, b, c) \
        case 0b1##a##b##c##0001: ret=((u64(*)(        char*, ...))f)((char*)p1[0], p##a[1], p##b[2], p##c[3]); break; \
        case 0b1##a##b##c##0010: ret=((u64(*)(int,    char*, ...))f)(p0[0], (char*)p1[1], p##a[2], p##b[3], p##c[4]); break; \
        case 0b1##a##b##c##0011: ret=((u64(*)(u64, char*, ...))f)(p1[0], (char*)p1[1], p##a[2], p##b[3], p##c[4]); break; \
        case 0b1##a##b##c##0100: ret=((u64(*)(int, int,       char*, ...))f)(p0[0], p0[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0101: ret=((u64(*)(int, u64,    char*, ...))f)(p0[0], p1[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0110: ret=((u64(*)(u64, int,    char*, ...))f)(p1[0], p0[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break; \
        case 0b1##a##b##c##0111: ret=((u64(*)(u64, u64, char*, ...))f)(p1[0], p1[1], (char*)p1[2], p##a[3], p##b[4], p##c[5]); break;
            
            FF;
            
        }
        Value v = NIL;
        switch (d->retType) {
        case CVOID:     v = NIL; break;            
        case CINT:      v = VAL_INT((int)  ret); break;
        case CLONG:     v = VAL_INT((long) ret); break;
        case CLONGLONG: v = VAL_INT(ret);        break;
        case CDOUBLE:   v = VAL_DOUBLE(*(double*)&ret); break;
        case CCONST|CPTR|CCHAR:
        case CPTR|CCHAR: {
            char *p = (char *) ret;
            v = String::makeVal(p, strlen(p));
            break;
        }
        case PTRDIFF: return VAL_INT(ret - p1[0]); break;
        }

        stack[0] = v;
    }
    return 0;
}


/*
        case   0b100001: ret=((u64(*)(char*, ...))f)(p[0]); break;
        case  0b1000001: ret=((u64(*)(char*, ...))f)(p[0],(int)p[1]); break;
        case  0b1100001: ret=((u64(*)(char*, ...))f)(p[0],p[1]); break;
        case 0b10000001: ret=((u64(*)(char*, ...))f)(p[0],(int)p[1],(int)p[2]); break;
        case 0b10100001: ret=((u64(*)(char*, ...))f)(p[0],(int)p[1],     p[2]); break;
        case 0b11000001: ret=((u64(*)(char*, ...))f)(p[0],     p[1],(int)p[2]); break;
        case 0b11100001: ret=((u64(*)(char*, ...))f)(p[0],     p[1],     p[2]); break;

        case 0b1000010: ret=((u64(*)(int,    char*, ...))f)(p[0], p[1], (int)p[2]);break;
        case 0b1000011: ret=((u64(*)(u64, char*, ...))f)(p[0], p[1], (int)p[2]);break;
        case 0b1100010: ret=((u64(*)(int,    char*, ...))f)(p[0], p[1], p[2]); break;
        case 0b1100011: ret=((u64(*)(u64, char*, ...))f)(p[0], p[1], p[2]); break;
*/
            /*
        case 0b1000: ret = ((u64 (*)(int, int, int)) f)(p[0], p[1], p[2]); break;
        case 0b1001: ret = ((u64 (*)(int, int, u64)) f)(p[0], p[1], p[2]); break;
        case 0b1010: ret = ((u64 (*)(int, u64, int)) f)(p[0], p[1], p[2]); break;
        case 0b1011: ret = ((u64 (*)(int, u64, u64)) f)(p[0], p[1], p[2]); break;
        case 0b1100: ret = ((u64 (*)(u64, int, int)) f)(p[0], p[1], p[2]); break;
        case 0b1101: ret = ((u64 (*)(u64, int, u64)) f)(p[0], p[1], p[2]); break;
        case 0b1110: ret = ((u64 (*)(u64, u64, int)) f)(p[0], p[1], p[2]); break;
        case 0b1111: ret = ((u64 (*)(u64, u64, u64)) f)(p[0], p[1], p[2]);break;
            */
/*
        case   0b1: 
            F1(0);
            F1(1);
            F2(0,0);
            F2(0,1);
            F2(1,0);
            F2(1,1);
            F3(0,0,0);
            F3(0,0,1);
            F3(0,1,0);
            F3(0,1,1);
            F3(1,0,0);
            F3(1,0,1);
            F3(1,1,0);
            F3(1,1,1);
*/
