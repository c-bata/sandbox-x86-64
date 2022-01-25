#ifndef CC_H_
#define CC_H_

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;

//
// tokenize.c
//

bool equal(Token *tok, char *s);
Token *skip(Token *tok, char *s);

typedef enum {
    TK_IDENT,   // Identifiers
    TK_PUNCT,   // Punctuators
    TK_KEYWORD, // Reserved keywords
    TK_NUM,     // Numeric literals
    TK_EOF,     // End-of-file marker
} TokenKind;

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

// Local variable
typedef struct Obj Obj;
struct Obj {
    Obj *next;
    char *name; // Variable name
    Type *ty;   // Type
    int offset; // Offset from RBP
};

// Function
typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Obj *params;
    Node *body;
    Obj *locals;
    int stack_size;
};

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
    ND_ADDR,      // unary &
    ND_DEREF,     // unary *
    ND_RETURN,    // return
    ND_IF,        // if
    ND_WHILE,     // while
    ND_FOR,       // for
    ND_BLOCK,     // { ... }
    ND_FUNCALL,   // Function call
    ND_EXPR_STMT, // Expression statement ';'
    ND_VAR,       // Variable
    ND_NUM,       // integer
} NodeKind;

struct Node {
    NodeKind kind; // node type
    Node *next;    // next node
    Node *lhs;     // left-hand side
    Node *rhs;     // right-hand side
    int val;       // used only if kind==ND_NUM
    Obj *var;      // used only if kind==ND_VAR
    Type *ty;      // Type. int or pointer

    // Block
    Node *body;

    // "if", "while", or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Function call
    char *funcname;
    Node *args;
};

Function *parse(Token *tok);


//
// type.c
//

typedef enum {
    TY_INT,
    TY_PTR,
    TY_FUNC,
} TypeKind;

struct Type {
    TypeKind kind;
    Type *base;   // Pointer
    Token *name;  // Declaration

    // for Function
    Type *return_ty;
    Type *params;  // function arg types
    Type *next;  // next argument type
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *copy_type(Type *src);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
void add_type(Node *node);

//
// codegen.c
//

typedef struct CodeGenOption CodeGenOption;
struct CodeGenOption {
    bool cpu_emu;
};

void codegen(Function* prog, CodeGenOption* option);

#endif
