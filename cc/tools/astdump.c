#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "stdio.h"
#include "../9cc.h"

// Avoid inserting the same graph node.
typedef struct Mark Mark;
struct Mark {
    int id_from;
    int id_to;
    char *label;
    Mark *next;
};
Mark mark_head = {};

static bool mark(int id_from, int id_to, char *label) {
    Mark *cur = &mark_head;
    while (cur->next) {
        cur = cur->next;
        if (cur->id_from == id_from &&
            cur->id_to == id_to &&
            strcmp(cur->label, label) == 0)
            return false;
    }

    Mark *new_mark = calloc(1, sizeof(Mark));
    new_mark->id_from = id_from;
    new_mark->id_to = id_to;
    new_mark->label = label;
    cur->next = new_mark;
    return true;
}

static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void assign_lvar_offsets(Obj *prog) {
    for (Obj *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (Obj *var = fn->locals; var; var = var->next) {
            offset += var->ty->size;
            var->offset = -offset;
        }
        fn->stack_size = align_to(offset, 16);
    }
}

static void gen_type(Type *ty) {
    if (ty->kind == TY_INT) {
        printf("%d [label=\"TY_INT\" color=green, style=filled]\n", (int) ty);
    } else if (ty->kind == TY_CHAR) {
        printf("%d [label=\"TY_CHAR\" color=green, style=filled]\n", (int) ty);
    } else if (ty->kind == TY_PTR) {
        printf("%d [label=\"TY_PTR\" color=green, style=filled]\n", (int) ty);
    } else if (ty->kind == TY_FUNC) {
        printf("%d [label=\"TY_FUNC\" color=green, style=filled]\n", (int) ty);
    } else if (ty->kind == TY_ARRAY) {
        printf("%d [label=\"TY_ARRAY[%d] (size=%d)\" color=green, style=filled]\n",
               (int) ty, ty->array_len, ty->size);
    }

    if (ty->base && mark((int) ty, (int) ty->base, "base")) {
        printf("%d -> %d [label=\"base\"]\n", (int) ty, (int) ty->base);
        gen_type(ty->base);
    }
    if (ty->return_ty && mark((int)ty, (int)ty->return_ty, "return_ty")) {
        printf("%d -> %d [label=\"return_ty\"]\n", (int) ty, (int) ty->return_ty);
        gen_type(ty->return_ty);
    }
    if (ty->params != NULL) {
        printf("%d -> %d [label=\"params\"]\n", (int) ty, (int) ty->params);
        gen_type(ty->params);
    }
    if (ty->next != NULL) {
        printf("%d -> %d [label=\"next\"]\n", (int) ty, (int) ty->next);
        gen_type(ty->next);
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
        printf("%d [label=\"FUNCALL %s()\" color=orange, style=filled]\n", (int) node, node->funcname);
    } else {
        printf("%d [label=\"%d\" color=orange, style=filled]\n", (int) node, node->kind);
    }

    // if (node->var != NULL) {
    //     printf("%d -> %d [label=\"var-ty\"]\n", (int) node, (int) node->var->ty);
    //     gen_type(node->var->ty);
    // }
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

static void gen_obj(Obj *obj) {
    printf("%d [label=\"Obj %s [rbp-%d]\" color=yellow, style=filled]\n",
           (int) obj, obj->name, -obj->offset);

    if (obj->ty && mark((int) obj, (int) obj->ty, "type")) {
        printf("%d -> %d [label=\"type\"]\n", (int) obj, (int) obj->ty);
        gen_type(obj->ty);
    }

    if (obj->next) {
        printf("%d -> %d [label=\"next\"]\n", (int) obj, (int) obj->next);
        gen_obj(obj->next);
    }
}

static void gen_global(Obj *global) {
    if (global->is_function) {
        printf("%d [label=\"Function %s (stack_size=%d)\" color=aquamarine, style=filled]\n",
               (int) global, global->name, global->stack_size);
        printf("%d -> %d [label=\"body\"]\n", (int) global, (int) global->body);
        gen_node(global->body);

        if (global->next) {
            printf("%d -> %d [label=\"next\"]\n", (int) global, (int) global->next);
            gen_global(global->next);
        }
        if (global->params) {
            printf("%d -> %d [label=\"params\"]\n", (int) global, (int) global->params);
            gen_obj(global->params);
        }
        if (global->locals) {
            printf("%d -> %d [label=\"locals\"]\n", (int) global, (int) global->locals);
            gen_obj(global->locals);
        }
    } else {
        // global variables
        printf("%d [label=\"Global Var %s\" color=aqua, style=filled]\n",
               (int) global, global->name);
        if (global->next)
            gen_global(global->next);
    }
    if (global->ty) {
        printf("%d -> %d [label=\"ty\"]\n", (int) global, (int) global->ty);
        gen_type(global->ty);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid arguments.");
        return 1;
    }

    Token *tok = tokenize(argv[1]);
    Obj *prog = parse(tok);
    assign_lvar_offsets(prog);

    printf("digraph g{\n");
    gen_global(prog);
    printf("}\n");
    return 0;
}
