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

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node* expr, Token *tok) {
    Node* node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

static Node *new_num(int val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_var(Obj* var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
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
// | program       = function*
// | function      = declspec declarator "{" compound-stmt
// | compound-stmt = (declaration | stmt)* "}"
// | declaration   = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
// | declspec      = "int"
// | type-suffix   = "(" func-params
// |               | "[" num "]" type-suffix
// |               | ε
// | func-params   = param ("," param)* ")"
// | param         = declspec declarator
// | declarator    = "*"* ident type-suffix
// | stmt          = expr ";" | "{" stmt* "}" | "return" expr ";"
// |                 | "if" "(" expr ")" stmt ("else" stmt)?
// |                 | "while" "(" expr ")" stmt
// |                 | "for" "(" expr? ";" expr? ";" expr? ")" stmt
// | expr          = assign
// | assign        = equality ("=" assign)?
// | equality      = relational ("==" relational | "!=" relational)*
// | relational    = add ("<" add | "<=" add | ">" add | ">=" add)*
// | add           = mul ("+" mul | "-" mul)*
// | mul           = unary ("*" unary | "/" unary)*
// | unary         = ("+" | "-" | "*" | "&")? unary | postfix
// | postfix       = primary ("[" expr "]")*
// | primary       = num | ident ("(" (assign ("," assign)*)? ")")? | "(" expr ")" | "sizeof" unary
// ↓
// High Priority

static Function *function(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);
static Type *declspec(Token **rest, Token *tok);
static Type *type_suffix(Token **rest, Token *tok, Type* ty);
static Type *func_params(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT)
        error_tok(tok, "expected an identifier");
    return my_strndup(tok->loc, tok->len);
}

static void create_param_lvars(Type *param) {
    if (param) {
        create_param_lvars(param->next);
        new_lvar(get_ident(param->name), param);
    }
}

static Function *function(Token **rest, Token *tok) {
    Type *functy = declspec(&tok, tok);
    functy = declarator(&tok, tok, functy);

    locals = NULL;
    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(functy->name);
    create_param_lvars(functy->params);
    fn->params = locals;

    tok = skip(tok, "{");
    fn->body = compound_stmt(&tok, tok);
    fn->locals = locals;
    *rest = tok;
    return fn;
}

static Node *compound_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_BLOCK, tok);

    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        if (equal(tok, "int"))
            cur = cur->next = declaration(&tok, tok);
        else
            cur = cur->next = stmt(&tok, tok);
        add_type(cur);
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

        Token *start = tok;
        Node *node = new_var(var, tok);
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), start);
        cur = cur->next = new_unary(ND_EXPR_STMT, node, start);
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = tok->next;
    return node;
}

static Type *declspec(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return ty_int;
}

static Type *type_suffix(Token **rest, Token *tok, Type* ty) {
    if (equal(tok, "(")) {
        ty = func_type(ty);
        ty->params = func_params(&tok, tok);
        *rest = tok;
        return ty;
    } else if (equal(tok, "[")) {
        if (tok->next->kind != TK_NUM)
            error_tok(tok->next, "expected a number");
        int size = tok->next->val;
        tok = skip(tok->next->next, "]");
        ty = type_suffix(&tok, tok, ty);
        *rest = tok;
        return array_of(ty, size);
    } else {
        return ty;
    }
}

static Type *func_params(Token **rest, Token *tok) {
    Type head = {};
    Type *cur = &head;
    tok = tok->next;
    while (!equal(tok, ")")) {
        if (cur != &head)
            tok = skip(tok, ",");

        Type *argty = declspec(&tok, tok);
        argty = declarator(&tok, tok, argty);
        cur = cur->next = copy_type(argty);
    }
    *rest = tok->next;
    return head.next;
}

static Type *declarator(Token **rest, Token *tok, Type *ty) {
    while(equal(tok, "*")) {
        ty = pointer_to(ty);
        tok = tok->next;
    }

    if (tok->kind != TK_IDENT)
        error_tok(tok, "expected a variable name");

    Token *ident_tok = tok;
    tok = tok->next;
    ty = type_suffix(&tok, tok, ty);
    ty->name = ident_tok;
    *rest = tok;
    return ty;
}

static Node *stmt(Token **rest, Token *tok) {
    Node *node;
    if (equal(tok, "{")) {
        // "{" stmt* "}"
        node = new_node(ND_BLOCK, tok);
        Node head = {};
        Node *cur = &head;
        tok = skip(tok, "{");
        while (!equal(tok, "}")) {
            cur = cur->next = stmt(&tok, tok);
        }
        node->body = head.next;
        *rest = tok->next;
        return node;
    }
    if (equal(tok, "if")) {
        node = new_node(ND_IF, tok);
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
        node = new_node(ND_WHILE, tok);
        tok = skip(tok, "while");
        tok = skip(tok, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }
    if (equal(tok, "for")) {
        node = new_node(ND_FOR, tok);
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
        node = new_unary(ND_RETURN, expr(&tok, tok->next), tok);
        *rest = skip(tok, ";");
        return node;
    }
    node = new_unary(ND_EXPR_STMT, expr(&tok, tok), tok);
    *rest = skip(tok, ";");
    return node;
}

static Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    Token *start = tok;
    for (;;) {
        if (equal(tok, "=")) {
            node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *equality(Token **rest, Token *tok) {
    Node* node = relational(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equal(tok, "==")) {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
            continue;;
        }
        *rest = tok;
        return node;
    }
}

static Node *relational(Token **rest, Token *tok) {
    Node* node = add(&tok, tok);
    for (;;) {
        Token *start = tok;
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next), start);
            continue;
        }
        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node, start);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node, start);
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
        return new_binary(ND_ADD, lhs, rhs, tok);

    if (lhs->ty->base && rhs->ty->base)
        error_tok(tok, "invalid operands");

    // Canonicalize `num + ptr` to `ptr + num`
    if (!lhs->ty->base && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
        add_type(rhs);
        Node *node = new_binary(ND_SUB, lhs, rhs, tok);
        node->ty = lhs->ty;
        return node;
    }

    // ptr-ptr, which returns how many elements are between the two.
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_binary(ND_SUB, lhs, rhs, tok);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
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
            node = new_binary(ND_MUL, node, unary(&tok, tok->next), tok);
            continue;
        }
        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next), tok);
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
        return new_unary(ND_NEG, unary(rest, tok->next), tok);
    }

    if (equal(tok, "&")) {
        return new_unary(ND_ADDR, unary(rest, tok->next), tok);
    }

    if (equal(tok, "*")) {
        return new_unary(ND_DEREF, unary(rest, tok->next), tok);
    }
    return postfix(rest, tok);
}

static Node *postfix(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    while (equal(tok, "[")) {
        // a[i] is a syntax sugar of *(a+i)
        Token *start = tok;
        Node *idx = expr(&tok, tok->next);
        node = new_unary(ND_DEREF, new_add(node, idx, tok), start);
        tok = skip(tok, "]");
    }
    *rest = tok;
    return node;
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
            Node *node = new_node(ND_FUNCALL, tok);
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
        Node *node = new_var(var, tok);
        *rest = tok->next;
        return node;
    }
    if (equal(tok, "sizeof")) {
        assert(tok->kind == TK_KEYWORD);
        Node *node = unary(&tok, tok->next);
        add_type(node);
        *rest = tok;
        return new_num(node->ty->size, tok);
    }
    if (tok->kind == TK_NUM) {
        Node *node = new_num(tok->val, tok);
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
