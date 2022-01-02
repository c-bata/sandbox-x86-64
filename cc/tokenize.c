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

// Generate a new token and set to cur->next.
static Token *new_token(TokenKind kind, char *start, char* end) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end- start;
    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">="))
        return 2;

    return ispunct(*p) ? 1 : 0;  // +-*/()<>;
}

Token *tokenize(char *p) {
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

        // Identifier
        if ('a' <= *p && *p <= 'z') {
            cur = cur->next = new_token(TK_IDENT, p, p+1);
            p++;
            continue;
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
    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}
