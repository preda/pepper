#include "Lexer.h"
#include "Vector.h"
#include "String.h"
#include "Value.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static bool isAlpha(char c)    { return ('a' <= (c|32) && (c|32) <= 'z') || c == '_'; }
static bool isDigit(char c)    { return '0' <= c && c <= '9'; }
static bool isAlphaNum(char c) { return isAlpha(c) || isDigit(c); }

static const char *tokens[] = {
    "<begin-keyword>",
    "break", "continue", 
    "else", "for", "while", "func", "goto",
    "if", "nil", "return", "var",
    "and", "or", "xor",
    "<end-keyword>",

    "<integer>", "<double>", "<name>", "<string>", "<end>",
};
//    "is",

Lexer::Lexer(const char *string) {
    this->string = (char *) string;
    p   = string;
    end = string + strlen(string);
    lineNumber = 0;
    for (int i = TK_BEGIN_KEYWORD; i < TK_END_KEYWORD; ++i) {
        HashEntry *e = keywords.add(hash64(tokens[i]));
        e->d = SymbolData(KIND_KEYWORD, 0, i);
    }
}

int Lexer::lookahead() {
    const char *savep = p;
    int saveLineNumber = lineNumber;
    TokenInfo dummy;
    int nextToken = advanceInt(&dummy);
    p = savep;
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
                u64 hash = 0;
                --p;
                while (isAlphaNum(c)) {
                    hash = hash64Step(c, hash);
                    c = *++p;
                }                
                if (HashEntry *e = keywords.get(hash)) {
                    return e->d.slot;
                }                
                info->nameHash = hash;
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

        case '&': return *p == '&' ? ++p, TK_LOG_AND : c;
        case '|': return *p == '|' ? ++p, TK_LOG_OR  : c;

        case '=': 
        case '!':
            return *p=='=' ? *++p=='=' ? (++p, c=='=' ? TK_IS : TK_NOT_IS) : c+TK_EQUAL : c;

        case ':': case '-': case '+':
            return *p == '=' ? ++p, c + TK_EQUAL : c;
                
        case '"':
            info->stringVal = readString();
            return TK_STRING;
        }
    }
    if (p >= end) {
        return TK_END;
    }
    return TK_END; // error
}

Value Lexer::readString() {
    char c;
    Vector<char> s;
    while (p < end && (c=*p) != '"') {
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
            }
        }
        s.push(c);
    }
    ERR(p >= end, E_OPEN_STRING);
    ++p;
    return String::makeVal(s.buf, s.size);
}
