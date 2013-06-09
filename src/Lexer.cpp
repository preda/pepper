#include "Lexer.h"
#include "Vector.h"
#include "String.h"
#include "Value.h"
#include "Map.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static bool isAlpha(char c)    { return ('a' <= (c|32) && (c|32) <= 'z') || c == '_'; }
static bool isDigit(char c)    { return '0' <= c && c <= '9'; }
static bool isAlphaNum(char c) { return isAlpha(c) || isDigit(c); }

const char *tokenStrings[] = {
#define _(x) #x
#include "tokens.inc"
#undef _
};

Lexer::Lexer(GC *gc, const char *string) :
    string(string),
    gc(gc),
    keywords(Map::alloc(gc))
{
    p   = string;
    pLine = p;
    end = string + strlen(string);
    lineNumber = 0;
    for (int i = 0; i < TK_END_KEYWORD; ++i) {
        keywords->indexSet(String::value(gc, tokenStrings[i]), VAL_REG(i));
    }
}

int Lexer::lookahead() {
    const char *savep = p;
    const char *savepLine = pLine;
    int saveLineNumber = lineNumber;
    TokenInfo dummy;
    int nextToken = advanceInt(&dummy);
    p = savep;
    pLine = savepLine;
    lineNumber = saveLineNumber;
    return nextToken;
}

int Lexer::advance() {
    // printf("before advance ");
    token = advanceInt(&info);
    return token;
}

int Lexer::advanceInt(TokenInfo *info) {
    while (p > (char*)2 && p < end) {
        char c = *p++;
        switch (c) {
        default:
            if (isAlpha(c)) {  
                --p;
                name.clear();
                while (isAlphaNum(c)) {
                    name.push(c);
                    c = *++p;
                }
                name.push(0);
                Value str = String::value(gc, name.buf());
                Value v = keywords->indexGet(str);
                if (!IS_NIL(v)) {
                    return (int) v;
                }
                info->name = str;
                return TK_NAME;
            } else if (isDigit(c)) {
                char *p1, *p2;
                s64 intVal       = strtoll(p-1, &p1, 0);
                double doubleVal =  strtod(p-1, &p2);
                if (p2 > p1) {
                    p = p2;
                    info->doubleVal = doubleVal;
                    return TK_DOUBLE;
                } else {
                    p = p1;
                    info->intVal = intVal;
                    return TK_INTEGER;
                }
            } else {
                return c;
            }
            
        case '\n': // newline
            ++lineNumber;
            pLine = p;
            break;
            
        case ' ': case '\f': case '\t': case '\v': // whitespace
            break;
            
        case '/': // comment
            c = *p;
            if (c == '/') {
                p = strchr(p+1, '\n') + 1;
            } else if (c == '*') {
                p = strstr(p+1, "*/") + 2;
            } else {
                return '/';
            }
            break;
            
        case '<': case '>':
            if (*p == '=' || *p == c) {
                int t = *p++ == '=' ? c + TK_EQUAL :
                    c == '<' ? TK_SHIFT_L : TK_SHIFT_R;
                return t;
            } else {
                return c;
            }

        case '&': return *p == '&' ? ++p, TK_and : c;
        case '|': return *p == '|' ? ++p, TK_or  : c;

        case '=': 
        case '!':
            return *p=='=' ? *++p=='=' ? (++p, c=='=' ? TK_IS : TK_NOT_IS) : c+TK_EQUAL : c;

        case ':': case '-': case '+':
            return *p == '=' ? ++p, c + TK_EQUAL : c;
                
        case '\'':
        case '"':
            info->name = readString(c);
            return TK_STRING;
        }
    }
    if (p >= end) {
        return TK_END;
    }
    return TK_END; // error
}

static char *makeMark(char *mark, int len, char at0, char atLen) {
    if (len <= 0) {
        mark[0] = '\'';
    } else {
        mark[0] = at0;
        mark[len] = atLen;
        memset(mark + 1, '=', len - 1);
    }
    mark[len + 1] = 0;
    return mark;
}

/* 0 => '
 * 1 => ]'
 * 3 => ]=='
 */
static char *makeEndMark(char *mark, int len) {
    return makeMark(mark, len, ']', '\'');
}

static char *makeBeginMark(char *mark, int len) {
    return makeMark(mark, len, '\'', '[');
}

// 'foo' '[foo]' '=[foo]=' etc.
char *Lexer::quote(char *buf, int bufSize, const char *str) {
    char endMark[32];
    char beginMark[32];
    for (int len = 0; len < 16; ++len) {
        makeEndMark(endMark, len);
        if (!strstr(str, endMark)) {
            makeBeginMark(beginMark, len);
            snprintf(buf, bufSize, "%s%s%s", beginMark, str, endMark);
            break;
        }
    }
    return buf;
}

Value Lexer::readString(char startChar) {
    int markLen = -1;
    if (startChar == '\'') {
        markLen = 0;
        const char *pp = p;
        while (pp < end && *pp == '=') {
            ++pp;
            ++markLen;
        }
        ERR(pp >= end, E_OPEN_STRING);
        if (*pp == '[') {
            ++markLen;
        } else {
            markLen = 0;
        }
    }
    Vector<char> s;
    if (markLen >= 0 && markLen < 128) {
        p += markLen;
        char endMark[132];
        makeEndMark(endMark, markLen);
        char *mark = (char *) memmem(p, end - p, endMark, markLen + 1);
        ERR(!mark, E_OPEN_STRING);
        s.append(p, mark - p);
        p = mark + (markLen + 1);
    } else {
        char c;
        while (p < end && (c=*p) != startChar) {
            ++p;
            if (c == '\\') {
                c = *p++;
                switch(c) {
                case 'a': c = '\a'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'v': c = '\v'; break;  
                case '"': c = '"';  break;
                case '\'': c = '\''; break;
                }
            }
            s.push(c);
        }
        ERR(p >= end, E_OPEN_STRING);
        ++p;
    }
    return String::value(gc, s.buf(), s.size());
}

void Lexer::printLocation() {
    const char *eol = strchr(pLine, '\n');
    if (!eol) { eol = p; }
    int lineSize = eol - pLine;
    char buf[lineSize + 1];
    strncpy(buf, pLine, lineSize);
    buf[lineSize] = 0;
    fprintf(stderr, "Line #%d : %d '%s'\n", lineNumber+1, (int)(p - pLine), buf);
}
