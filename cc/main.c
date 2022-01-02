#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

bool emu_mode = false;

int opt_remove_at(int argc, char* argv[], int index) {
    if (index < 0 || argc <= index) {
        return argc;
    } else {
        int i = index;
        for (; i<argc-1; i++) {
            argv[i] = argv[i+1];
        }
        argv[i] = NULL;
        return argc -1;
    }
}

typedef enum {
    TK_PUNCT, // Punctuators
    TK_NUM,   // Numeric literals
    TK_EOF,   // End-of-file marker
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;  // Token type
    Token *next;     // Next token
    int val;         // Used only if kind==TK_NUM
    char *loc;       // Token location
    int len;         // Token length
};

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM, // integer
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind; // node type
    Node *lhs;     // left-hand side
    Node *rhs;     // right-hand side
    int val;       // used only if kind==ND_NUM
};

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

static void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

static bool equal(Token *tok, char *s) {
    return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}

// Ensure that the current token is `s`.
static Token *skip(Token *tok, char *s) {
    if (!equal(tok, s))
        error_tok(tok, "expected '%s'", s);
    return tok->next;
}

// Generate a new token and set to cur->next.
Token *new_token(TokenKind kind, char *start, char* end) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end- start;
    return tok;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">="))
        return 2;

    return ispunct(*p) ? 1 : 0;
}

Token *tokenize() {
    char *p = current_input;
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

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// EBNF
// | expr       = equality
// | equality   = relational ("==" relational | "!=" relational)*
// | relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// | add        = mul ("+" mul | "-" mul)*
// | mul        = unary ("*" unary | "/" unary)*
// | unary      = ("+" | "-")? primary
// | primary    = num | "(" expr ")"
// ↓
// High Priority

Node *expr(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
}

Node *equality(Token **rest, Token *tok) {
    Node* node = relational(&tok, tok);
    for (;;) {
        if (equal(tok, "==")) {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_binary(ND_NE, node, relational(&tok, tok->next));
            continue;;
        }
        *rest = tok;
        return node;
    }
}

Node *relational(Token **rest, Token *tok) {
    Node* node = add(&tok, tok);
    for (;;) {
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node);
            continue;
        }
        *rest = tok;
        return node;
    }
}

Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        if (equal(tok, "+")) {
            node = new_binary(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        if (equal(tok, "-")) {
            node = new_binary(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);
    for (;;) {
        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }
        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return unary(rest, tok->next);

    if (equal(tok, "-"))
        return new_binary(ND_SUB, new_node_num(0), unary(rest, tok->next));
    return primary(rest, tok);
}

Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_NUM) {
        Node *node = new_node_num(tok->val);
        *rest = tok->next;
        return node;
    }
    error_tok(tok, "expected an expression");
}

void gen_expr(Node* node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            printf("  cmp rax, rdi\n");
            if (node->kind == ND_EQ)
                printf("  sete al\n");
            else if (node->kind == ND_NE)
                printf("  setne al\n");
            else if (node->kind == ND_LT)
                printf("  setl al\n");
            else if (node->kind == ND_LE)
                printf("  setle al\n");
#ifdef __linux
            printf("  movzb rax, al\n");
#else
            printf("  movzx rax, al\n");
#endif
            break;
    }
    printf("  push rax\n");
}

int main(int argc, char **argv) {
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--cpuemu") == 0) {
            emu_mode = true;
            argc = opt_remove_at(argc, argv, i);
        } else {
            i++;
        }
    }

    if (argc != 2) {
        error("error: no input program.");
    }

    current_input = argv[1];
    Token *tok = tokenize();
    Node *node = expr(&tok, tok);

    if (tok->kind != TK_EOF)
        error_tok(tok, "extra token");

    if (emu_mode) {
        printf("%%define movzb movzx\n");  // Macro for NASM
        printf("BITS 64\n");
        printf("  org 0x7c00\n");
    } else {
        printf(".intel_syntax noprefix\n");
#ifdef __linux
        printf(".global main\n");
        printf("main:\n");
#else  // macOS
        printf(".global _main\n");
        printf("_main:\n");
#endif
    }

    gen_expr(node);

    printf("  pop rax\n");
    if (emu_mode)
        printf("  jmp 0\n");
    else
        printf("  ret\n");
    return 0;
}
