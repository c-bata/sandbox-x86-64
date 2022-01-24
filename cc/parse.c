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
    Node *node = new_node(ND_NUM);
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
// | stmt       = expr ";" | "{" stmt* "}" | "return" expr ";"
// |              | "if" "(" expr ")" stmt ("else" stmt)?
// |              | "while" "(" expr ")" stmt
// |              | "for" "(" expr? ";" expr? ";" expr? ")" stmt
// | expr       = assign
// | assign     = equality ("=" assign)?
// | equality   = relational ("==" relational | "!=" relational)*
// | relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// | add        = mul ("+" mul | "-" mul)*
// | mul        = unary ("*" unary | "/" unary)*
// | unary      = ("+" | "-" | "*" | "&")? unary | primary
// | primary    = num | ident ("(" (assign ("," assign)*)? ")")? | "(" expr ")"
// ↓
// High Priority

static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static Node *stmt(Token **rest, Token *tok) {
    Node *node;
    if (equal(tok, "{")) {
        // "{" stmt* "}"
        Node head = {};
        Node *cur = &head;
        tok = skip(tok, "{");
        while (!equal(tok, "}")) {
            cur = cur->next = stmt(&tok, tok);
        }
        node = new_node(ND_BLOCK);
        node->body = head.next;
        *rest = tok->next;
        return node;
    }
    if (equal(tok, "if")) {
        node = new_node(ND_IF);
        tok = skip(tok, "if");
        tok = skip(tok, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else")) {
            node->els = stmt(&tok, tok->next);
        }
        *rest = tok;
        return node;
    }
    if (equal(tok, "while")) {
        node = new_node(ND_WHILE);
        tok = skip(tok, "while");
        tok = skip(tok, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }
    if (equal(tok, "for")) {
        node = new_node(ND_FOR);
        tok = skip(tok, "for");
        tok = skip(tok, "(");
        if (!equal(tok, ";"))
            node->init = expr(&tok, tok);
        tok = skip(tok, ";");
        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");
        if (!equal(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = skip(tok, ")");

        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }
    if (equal(tok, "return")) {
        node = new_unary(ND_RETURN, expr(&tok, tok->next));
        *rest = skip(tok, ";");
        return node;
    }
    node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

static Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

static Node *assign(Token **rest, Token *tok) {
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

static Node *equality(Token **rest, Token *tok) {
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

static Node *relational(Token **rest, Token *tok) {
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

static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs);

    if (lhs->ty->base && rhs->ty->base)
        error_tok(tok, "invalid operands");

    // Canonicalize `num + ptr` to `ptr + num`
    if (!lhs->ty->base && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_binary(ND_MUL, rhs, new_node_num(8));
    return new_binary(ND_ADD, lhs, rhs);
}

static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        if (equal(tok, "+")) {
            node = new_add(node, mul(&tok, tok->next), tok);
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

static Node *mul(Token **rest, Token *tok) {
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

static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return unary(rest, tok->next);

    if (equal(tok, "-")) {
        return new_unary(ND_NEG, unary(rest, tok->next));
    }

    if (equal(tok, "&")) {
        return new_unary(ND_ADDR, unary(rest, tok->next));
    }

    if (equal(tok, "*")) {
        return new_unary(ND_DEREF, unary(rest, tok->next));
    }
    return primary(rest, tok);
}

static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        // Function call
        if (equal(tok->next, "(")) {
            Node *node = new_node(ND_FUNCALL);
            node->funcname = my_strndup(tok->loc, tok->len);
            tok = skip(tok->next, "(");

            Node head = {};
            Node *cur = &head;
            while (!equal(tok, ")")) {
                if (cur != &head)
                    tok = skip(tok, ",");
                cur = cur->next = assign(&tok, tok);
            }
            node->args = head.next;

            *rest = skip(tok, ")");
            return node;
        }

        // Variable
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
