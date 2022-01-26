#include <assert.h>
#include <string.h>
#include "stdio.h"
#include "9cc.h"

static int depth;
static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static Function *current_fn;

static void gen_expr(Node* node);

static void push(void) {
    printf("  push rax\n");
    depth++;
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
    depth--;
}

static int gen_unique_num(void) {
    static int i = 1;
    return i++;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
    switch (node->kind) {
        case ND_VAR:
            assert(node->var != NULL);
            // "lea dst, [src]" calculates the absolute address of src,
            // then load it into rax.
            printf("  lea rax, [rbp-%d]\n", -node->var->offset);
            return;
        case ND_DEREF:
            // ex) *x = 5;
            gen_expr(node->lhs);  // pointer x (address) -> rax
            return;
        default:
            error("not an lvalue");
    }
}

// Load a value from where rax is pointing to.
static void load(Type *ty) {
    if (ty->kind == TY_ARRAY) {
        // If it is an array, do not attempt to load a value to the
        // register because in general we can't load an entire array to a
        // register. As a result, the result of an evaluation of an array
        // becomes not the array itself but the address of the array.
        // This is where "array is automatically converted to a pointer to
        // the first element of the array in C" occurs.
        return;
    }
    printf("  mov rax, [rax]\n");  // [address] -> rax
}

static void gen_expr(Node* node) {
    switch (node->kind) {
        case ND_NUM:
            printf("  mov rax, %d\n", node->val);
            return;
        case ND_NEG:
            gen_expr(node->lhs);
            printf("  neg rax\n");
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs); // lvar address -> rax
            push();  // address -> stack
            gen_expr(node->rhs);  // rvalue -> rax
            pop("rdi");  // address -> rdi
            printf("  mov [rdi], rax\n");  // rvalue -> [address]
            return;
        case ND_VAR:
            gen_addr(node); // address -> rax
            assert(node->ty);
            load(node->ty);
            return;
        case ND_ADDR:
            gen_addr(node->lhs);  // address -> rax
            return;
        case ND_DEREF:
            gen_expr(node->lhs);  // address -> rax
            assert(node->ty);
            load(node->ty);
            return;
        case ND_FUNCALL: {
            printf("  mov rax, 0\n");

            int nargs = 0;
            for (Node *arg = node->args; arg; arg = arg->next) {
                gen_expr(arg);
                push();
                nargs++;
            }

            for (int i = nargs-1; i>=0; i--)
                pop(argreg[i]);

#ifdef __linux
            printf("  call %s\n", node->funcname); // ret -> rax
#else
            printf("  call _%s\n", node->funcname); // ret -> rax
#endif
            return;
        }
    }

    gen_expr(node->rhs); // rhs -> rax
    push();  // rhs -> stack
    gen_expr(node->lhs); // lhs -> rax
    pop("rdi");  // rhs -> rdi

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            return;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            return;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            return;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            return;
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
            return;
    }
    error("invalid expression");
}

static void gen_stmt(Node* node) {
    int unique_label;
    switch (node->kind) {
        case ND_BLOCK:
            for (Node *n = node->body; n; n=n->next)
                gen_stmt(n);
            return;
        case ND_EXPR_STMT:
            gen_expr(node->lhs);
            return;
        case ND_RETURN:
            gen_expr(node->lhs);
            printf("  jmp .L.return.%s\n", current_fn->name);
            return;
        case ND_IF:
            gen_expr(node->cond);  // cond -> rax
            printf("  cmp rax, 0\n");
            unique_label = gen_unique_num();
            if (node->els == NULL) {
                printf("  je .Lend%d\n", unique_label);
                gen_stmt(node->then);
                printf(".Lend%d:\n", unique_label);
            } else {
                printf("  je .Lels%d\n", unique_label);
                gen_stmt(node->then);
                printf("  jmp .Lend%d\n", unique_label);
                printf(".Lels%d:\n", unique_label);
                gen_stmt(node->els);
                printf(".Lend%d:\n", unique_label);
            }
            return;
        case ND_WHILE:
            unique_label = gen_unique_num();
            printf(".Lbegin%d:\n", unique_label);
            gen_expr(node->cond);
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", unique_label);
            gen_stmt(node->then);
            printf("  jmp .Lbegin%d\n", unique_label);
            printf(".Lend%d:\n", unique_label);
            return;
        case ND_FOR:
            unique_label = gen_unique_num();
            if (node->init != NULL)
                gen_expr(node->init);
            printf(".Lbegin%d:\n", unique_label);
            if (node->cond != NULL) {
                gen_expr(node->cond);
                printf("  cmp rax, 0\n");
                printf("  je .Lend%d\n", unique_label);
            }
            gen_stmt(node->then);
            if (node->inc != NULL)
                gen_expr(node->inc);
            printf("  jmp .Lbegin%d\n", unique_label);
            printf(".Lend%d:\n", unique_label);
            return;
    }
    error("invalid statement: %d", node->kind);
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void assign_lvar_offsets(Function *prog) {
    for (Function *fn = prog; fn; fn = fn->next) {
        int offset = 0;
        for (Obj *var = fn->locals; var; var = var->next) {
            offset += var->ty->size;
            var->offset = -offset;
        }
        fn->stack_size = align_to(offset, 16);
    }
}

void codegen(Function* prog, CodeGenOption* option) {
    assign_lvar_offsets(prog);
    printf(".intel_syntax noprefix\n");
    for (Function *fn = prog; fn; fn = fn->next) {

#ifdef __linux
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
#else  // macOS
        printf(".global _%s\n", fn->name);
        printf("_%s:\n", fn->name);
#endif
        current_fn = fn;

        // Prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", fn->stack_size);

        int nparams = 0;
        for (Obj *param = fn->params; param; param = param->next)
            printf("  mov [rbp-%d], %s\n", -param->offset, argreg[nparams++]);

        gen_stmt(fn->body);
        assert(depth == 0);
        assert(fn->body->next == NULL);

        // Epilogue
        printf(".L.return.%s:\n", fn->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}
