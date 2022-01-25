#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

static Obj *new_lvar(char *name, Type *ty) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->next = locals;
    var->ty = ty;
    locals = var;
    return var;
}

// EBNF
// | program    = function-definition*
// | function-definition = declspec declarator "{" compound-stmt
// | compound-stmt = (declaration | stmt)* "}"
// | declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
// | declspec = "int"
// | type-suffix = ("(" func-params ")")?
// | declarator = "*"* ident type-suffix
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

static Function *function(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);
static Type *declspec(Token **rest, Token *tok);
static Type *type_suffix(Token **rest, Token *tok, Type* ty);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT)
        error_tok(tok, "expected an identifier");
    return my_strndup(tok->loc, tok->len);
}

static Function *function(Token **rest, Token *tok) {
    Type *functy = declspec(&tok, tok);
    functy = declarator(&tok, tok, functy);

    locals = NULL;
    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(functy->name);

    tok = skip(tok, "{");
    fn->body = compound_stmt(&tok, tok);
    fn->locals = locals;
    *rest = tok;
    return fn;
}

static Node *compound_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_BLOCK);

    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        if (equal(tok, "int"))
            cur = cur->next = declaration(&tok, tok);
        else
            cur = cur->next = stmt(&tok, tok);
    }

    node->body = head.next;
    *rest = tok->next;
    return node;
}

static Node *declaration(Token **rest, Token *tok) {
    Type *basety = declspec(&tok, tok);

    Node head = {};
    Node *cur = &head;
    int i = 0;

    while(!equal(tok, ";")) {
        if (i++ > 0)
            tok = skip(tok, ",");

        Type *ty = declarator(&tok, tok, basety);
        Obj *var = new_lvar(get_ident(ty->name), ty);

        if (!equal(tok, "="))
            continue;

        Node *node = new_var_node(var);
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
        cur = cur->next = new_unary(ND_EXPR_STMT, node);
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    *rest = tok->next;
    return node;
}

static Type *declspec(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return ty_int;
}

static Type *type_suffix(Token **rest, Token *tok, Type* ty) {
    // type-suffix = ("()")?
    if (!equal(tok, "("))
        return ty;
    *rest = skip(tok->next, ")");
    return func_type(ty);
}

static Type *declarator(Token **rest, Token *tok, Type *ty) {
    while(equal(tok, "*")) {
        ty = pointer_to(ty);
        tok = tok->next;
    }

    if (tok->kind != TK_IDENT)
        error_tok(tok, "expected a variable name");

    Token *ident_tok = tok;
    ty = type_suffix(&tok, tok->next, ty);
    ty->name = ident_tok;
    *rest = tok;
    return ty;
}

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

static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs);

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_binary(ND_MUL, rhs, new_node_num(8));
        add_type(rhs);
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = lhs->ty;
        return node;
    }

    // ptr-ptr, which returns how many elements are between the two.
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_node_num(8));
    }
    error_tok(tok, "invalid operations");
}

static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        if (equal(tok, "+")) {
            node = new_add(node, mul(&tok, tok->next), tok);
            continue;
        }
        if (equal(tok, "-")) {
            node = new_sub(node, mul(&tok, tok->next), tok);
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
    Function head = {};
    Function *cur = &head;
    while (tok->kind != TK_EOF)
        cur = cur->next = function(&tok, tok);
    return head.next;
}
