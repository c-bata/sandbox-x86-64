#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "9cc.h"

// Input program
static char *current_input;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

bool equal(Token *tok, char *s) {
    return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}

// Ensure that the current token is `s`.
Token *skip(Token *tok, char *s) {
    if (!equal(tok, s))
        error_tok(tok, "expected '%s'", s);
    return tok->next;
}

// Generate a new token and set to cur->next.
static Token *new_token(TokenKind kind, char *start, char* end) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

static bool is_alphabet(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
    return is_alphabet(c) || ('0' <= c && c <= '9');
}

static bool is_keyword(Token *tok) {
    static char *kw[] = {"return", "if", "else", "for", "while", "int", "char", "sizeof"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
        if (equal(tok, kw[i]))
            return true;
    return false;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">=") ||
        startswith(p, "++") || startswith(p, "--") ||
        startswith(p, "+=") || startswith(p, "-=") ||
        startswith(p, "*=") || startswith(p, "/="))
        return 2;

    return ispunct(*p) ? 1 : 0;  // +-*&/(){}<>;
}

static void convert_keywords(Token *tok) {
    for (Token *t = tok; t->kind != TK_EOF; t = t->next)
        if (is_keyword(t))
            t->kind = TK_KEYWORD;
}

static int read_escape_char(char *p) {
    // Escape sequences are defined using themselves here. This tautological definition
    // works because the compiler that compiles our compiler knows what '\n' actually is.
    // E.g. '\n' is implemented using '\n'.
    switch (*p) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        // [GNU] \e for the ASCII escape character is a GNU C extension.
        case 'e': return 27;
        default: return *p;
    }
}

Token *tokenize_file(char *p) {
    current_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // Skip whitespace characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Numeric literal
        if (isdigit(*p)) {
            cur = cur->next = new_token(TK_NUM, p, 0);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // Identifier or Keywords
        if (is_alphabet(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_alnum(*p));
            cur = cur->next = new_token(TK_IDENT, start, p);
            continue;
        }

        // Read string
        if (*p == '"') {
            char *start = p;
            int strlen = 1;  // for '\0'
            while(*(++p) != '"') {
                if (*p == '\n' || *p == '\0')
                    error_at(start, "unclosed string literal");
                if (*p == '\\')
                    p++;
                strlen++;
            }
            p++;  // skip a double quote.

            char *buf = calloc(1, strlen);
            int i = 0;
            for (char *q = start + 1; q < p; q++) {
                if (*q == '\\')
                    buf[i++] = read_escape_char(++q);
                else
                    buf[i++] = *q;
            }
            buf[strlen-1] = '\0';

            Token *tok = new_token(TK_STR, start, p);
            tok->str = buf;
            tok->strlen = strlen;
            cur = cur->next = tok;
        }

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len) {
            cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
            p += cur->len;
            continue;
        }
        error_at(p, "tokenizer error: '%c'\n", *p);
    }
    cur->next = new_token(TK_EOF, p, p);
    convert_keywords(head.next);
    return head.next;
}
