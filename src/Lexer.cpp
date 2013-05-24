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
        keywords->rawSet(String::value(gc, tokenStrings[i]), VAL_REG(i));
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
                Value v = keywords->rawGet(str);
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

Value Lexer::readString(char startChar) {
    int markLen = -1;
    if (startChar == '\'') {
        markLen = 0;
        const char *pp = p;
        while (pp < end && *pp == '=') {
            ++pp;
            ++markLen;
        }
        if (pp >= end || *pp != '[') {
            markLen = -1;
        }
    }
    Vector<char> s;
    if (markLen >= 0 && markLen < 128) {
        p += markLen + 1;
        char endMark[130];
        endMark[0] = ']';
        memset(endMark + 1, '=', markLen);
        endMark[markLen+1] = '\'';
        endMark[markLen+2] = 0;
        char *mark = (char *) memmem(p, end - p, endMark, markLen + 2);
        ERR(!mark, E_OPEN_STRING);
        s.append(p, mark - p);
        p = mark + (markLen + 2);
        // fprintf(stderr, "******* %s", p);
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
