#include "stdio.h"
#include "../9cc.h"

static void gen_node(Node *node) {
    if (node->kind == ND_VAR) {
        printf("%d [label=\"VAR %c\" color=orange, style=filled]\n", (int) node, node->name);
    } else if (node->kind == ND_ASSIGN) {
        printf("%d [label=\"ASSIGN\" color=orange, style=filled]\n", (int) node);
    } else if (node->kind == ND_NUM) {
        printf("%d [label=\"NUM %d\" color=orange, style=filled]\n", (int) node, node->val);
    //} else if (node->kind == ND_RETURN) {
    //    printf("%d [label=\"RETURN\" color=orange, style=filled]\n", (int) node);
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
    } else {
        printf("%d [label=\"%d\" color=orange, style=filled]\n", (int) node, node->kind);
    }


    if (node->lhs != NULL) {
        printf("%d -> %d\n", (int) node, (int) node->lhs);
        gen_node(node->lhs);
    }
    if (node->rhs != NULL) {
        printf("%d -> %d\n", (int) node, (int) node->rhs);
        gen_node(node->rhs);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid arguments.");
        return 1;
    }

    Token *tok = tokenize(argv[1]);
    Node *node = parse(tok);

    printf("digraph g{\n");
    for (Node *n=node; n; n=n->next) {
        gen_node(n);
    }
    printf("}\n");
    return 0;
}
