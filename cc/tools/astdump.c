#include <assert.h>
#include "stdio.h"
#include "../9cc.h"

static void gen_type(Type *ty) {
    if (ty->kind == TY_INT) {
        printf("%d [label=\"Type INT\" color=green, style=filled]\n", (int) ty);
    } else if (ty->kind == TY_PTR) {
        printf("%d [label=\"Type PTR\" color=green, style=filled]\n", (int) ty);
    }
    if (ty->base != NULL) {
        printf("%d -> %d [label=\"base\"]\n", (int) ty, (int) ty->base);
        gen_type(ty->base);
    }
}

static void gen_node(Node *node) {
    if (node->kind == ND_VAR) {
        assert(node->var != NULL);
        printf("%d [label=\"VAR %s\" color=orange, style=filled]\n", (int) node, node->var->name);
    } else if (node->kind == ND_ASSIGN) {
        printf("%d [label=\"=\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_NUM) {
        printf("%d [label=\"NUM %d\" color=orange, style=filled]\n", (int) node, node->val);
    } else if (node->kind == ND_RETURN) {
        printf("%d [label=\"RETURN\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_EXPR_STMT) {
        printf("%d [label=\"EXPR_STMT\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_LE) {
        printf("%d [label=\"<=\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_LT) {
        printf("%d [label=\"<\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_EQ) {
        printf("%d [label=\"==\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_NE) {
        printf("%d [label=\"!=\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_ADD) {
        printf("%d [label=\"+\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_SUB) {
        printf("%d [label=\"-\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_MUL) {
        printf("%d [label=\"*\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_DIV) {
        printf("%d [label=\"/\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_ADDR) {
        printf("%d [label=\"ADDR &\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_DEREF) {
        printf("%d [label=\"DEREF *\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_BLOCK) {
        printf("%d [label=\"{ BLOCK }\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_IF) {
        printf("%d [label=\"IF\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_WHILE) {
        printf("%d [label=\"WHILE\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_FOR) {
        printf("%d [label=\"FOR\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_FUNCALL) {
        printf("%d [label=\"%s()\" color=orange, style=filled]\n", (int) node, node->funcname);
    } else {
        printf("%d [label=\"%d\" color=orange, style=filled]\n", (int) node, node->kind);
    }

    if (node->ty != NULL) {
        printf("%d -> %d [label=\"ty\"]\n", (int) node, (int) node->ty);
        gen_type(node->ty);
    }
    if (node->lhs != NULL) {
        printf("%d -> %d [label=\"lhs\"]\n", (int) node, (int) node->lhs);
        gen_node(node->lhs);
    }
    if (node->rhs != NULL) {
        printf("%d -> %d [label=\"rhs\"]\n", (int) node, (int) node->rhs);
        gen_node(node->rhs);
    }
    if (node->body != NULL) {
        printf("%d -> %d [label=\"body\"]\n", (int) node, (int) node->body);
        gen_node(node->body);
    }
    if (node->init != NULL) {
        printf("%d -> %d [label=\"init\"]\n", (int) node, (int) node->init);
        gen_node(node->init);
    }
    if (node->cond != NULL) {
        printf("%d -> %d [label=\"cond\"]\n", (int) node, (int) node->cond);
        gen_node(node->cond);
    }
    if (node->inc != NULL) {
        printf("%d -> %d [label=\"inc\"]\n", (int) node, (int) node->inc);
        gen_node(node->inc);
    }
    if (node->then != NULL) {
        printf("%d -> %d [label=\"then\"]\n", (int) node, (int) node->then);
        gen_node(node->then);
    }
    if (node->els != NULL) {
        printf("%d -> %d [label=\"els\"]\n", (int) node, (int) node->els);
        gen_node(node->els);
    }
    if (node->args != NULL) {
        printf("%d -> %d [label=\"args\"]\n", (int) node, (int) node->args);
        gen_node(node->args);
    }
    if (node->next != NULL) {
        printf("%d -> %d [label=\"next\"]\n", (int) node, (int) node->next);
        gen_node(node->next);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid arguments.");
        return 1;
    }

    Token *tok = tokenize(argv[1]);
    Function *prog = parse(tok);

    printf("digraph g{\n");
    gen_node(prog->body);
    printf("}\n");
    return 0;
}
