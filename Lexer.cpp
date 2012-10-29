#include "Lexer.h"
#include "Vector.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Lexer::Lexer(char *string) {
    this->string = (char *) string;
    p   = string;
    end = string + strlen(string);
    lineNumber = 0;
    error = 0;
    errorPos = 0;
}

int Lexer::nextToken(TokenInfo *info) {
    if (error) {
        return TK_ERROR;
    }
    while (p > (char*)2 && p < end) {
        char c = *p++;
        switch (c) {
        default:
            if (isAlpha(c)) {
                Vector<char> s;
                s.push(c);
                c = *p;
                while (isAlphaNum(c)) {
                    s.push(c);
                    c = *++p;
                }
                s.push('\0');
                char *tmp = s.buf;
                for (int i = TK_BEGIN_KEYWORD; i < TK_END_KEYWORD; ++i) {
                    if (!strcmp(tmp, tokens[i])) {
                        return i;
                    }
                }
                info->string = strdup(tmp);
                return TK_NAME;
            } else if (isDigit(c)) {
                char p1, p2;
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
            
        case '=': case '<': case '>': case '!': case ':': case '-': case '+':
            return *p == '=' ? ++p, c + TK_EQUAL : c;
                
        case '"':
            info->string = readString();
            return TK_STRING;
        }
    }
    if (p >= end) {
        return TK_EOS;
    }
}

char *Lexer::readString() {
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
    if (++p > end) {
        return 0;
    }
    s.push('\0');
    char *ret = strdup(s.buf);
    return ret;
}
