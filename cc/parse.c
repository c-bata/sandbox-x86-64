#include <stdlib.h>
#include <string.h>
#include "9cc.h"

// All local variable instances created during parsing are
// accumulated to this list.
Obj *locals;

char* my_strndup(const char* s, int len) {
    // Write my own strndup function to avoid
    // 'strndup.c no such file or directory' error.
    char* dup = malloc(sizeof(char)*(len+1));
    int i;
    for (i=0; i<len; i++) {
        dup[i] = s[i];
    }
    dup[len] = '\0';
    return dup;
}

// Find a local variable by name.
static Obj *find_var(Token *tok) {
    for (Obj *var = locals; var; var = var->next)
        if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len))
            return var;
    return NULL;
}

static Node *new_node(NodeKind kind) {
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

static Node *new_unary(NodeKind kind, Node* expr) {
    Node* node = new_node(kind);
    node->lhs = expr;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node *new_var_node(Obj* var) {
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

static Obj *new_lvar(char *name) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->next = locals;
    locals = var;
    return var;
}

// EBNF
// | program    = stmt*
// | stmt       = expr ";" | "return" expr ";"
// | expr       = assign
// | assign     = equality ("=" assign)?
// | equality   = relational ("==" relational | "!=" relational)*
// | relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// | add        = mul ("+" mul | "-" mul)*
// | mul        = unary ("*" unary | "/" unary)*
// | unary      = ("+" | "-")? primary
// | primary    = num | ident | "(" expr ")"
// ↓
// High Priority

Node *stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
Node *assign(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

Node *stmt(Token **rest, Token *tok) {
    Node *node;
    if (equal(tok, "return"))
        node = new_unary(ND_RETURN, expr(&tok, tok->next));
    else
        node = expr(&tok, tok);
    node = new_unary(ND_EXPR_STMT, node);
    *rest = skip(tok, ";");
    return node;
}

Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    for (;;) {
        if (equal(tok, "=")) {
            node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
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

    if (equal(tok, "-")) {
        return new_unary(ND_NEG, unary(rest, tok->next));
    }
    return primary(rest, tok);
}

Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        Obj *var = find_var(tok);
        if (!var)
            var = new_lvar(my_strndup(tok->loc, tok->len));
        Node *node = new_var_node(var);
        *rest = tok->next;
        return node;
    }
    if (tok->kind == TK_NUM) {
        Node *node = new_node_num(tok->val);
        *rest = tok->next;
        return node;
    }
    error_tok(tok, "expected an expression");
}

Function *parse(Token *tok) {
    Node head = {};
    Node *cur = &head;

    // program = stmt*
    while (tok->kind != TK_EOF)
        cur = cur->next = stmt(&tok, tok);

    Function *prog = calloc(1, sizeof(Function));
    prog->body = head.next;
    prog->locals = locals;
    return prog;
}
