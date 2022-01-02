#ifndef CC_H_
#define CC_H_

#include <stdbool.h>

//
// tokenize.c
//

typedef enum {
    TK_IDENT, // Identifiers
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

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

Token *tokenize(char *p);

//
// parse.c
//

typedef enum {
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_NEG,       // unary -
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_EXPR_STMT, // Expression statement ';'
    ND_VAR,       // Variable
    ND_NUM,       // integer
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind; // node type
    Node *next;    // next node
    Node *lhs;     // left-hand side
    Node *rhs;     // right-hand side
    int val;       // used only if kind==ND_NUM
    char name;     // used only if kind==ND_VAR
};

Node *parse(Token *tok);

//
// codegen.c
//

typedef struct CodeGenOption CodeGenOption;
struct CodeGenOption {
    bool cpu_emu;
};

void codegen(Node *node, CodeGenOption* option);

#endif
