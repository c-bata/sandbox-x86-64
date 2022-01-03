#include <assert.h>
#include "stdio.h"
#include "9cc.h"

static int depth;
static bool emu_mode = false;

static void push(void) {
    printf("  push rax\n");
    depth++;
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
    depth--;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        assert(node->var != NULL);
        // "lea dst, [src]" calculates the absolute address of src,
        // then load it into rax.
        printf("  lea rax, [rbp-%d]\n", -node->var->offset);
        return;
    }
    error("not an lvalue");
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
            printf("  mov rax, [rax]\n");  // [address] -> rax
            return;
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
    if (node->kind == ND_EXPR_STMT) {
        gen_expr(node->lhs);
        return;
    }
    error("invalid statement");
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void assign_lvar_offsets(Function *prog) {
    int offset = 0;
    for (Obj *var = prog->locals; var; var=var->next) {
        offset += 8;
        var->offset = -offset;
    }
    prog->stack_size = align_to(offset, 16);
}

void codegen(Function* prog, CodeGenOption* option) {
    assign_lvar_offsets(prog);

    emu_mode = option->cpu_emu;
    if (emu_mode) {
        printf("%%define movzb movzx\n");  // Macro for NASM
        printf("BITS 64\n");
        printf("  org 0x7c00\n");
    } else {
        printf(".intel_syntax noprefix\n");
#ifdef __linux
        printf(".global main\n");
        printf("main:\n");
#else  // macOS
        printf(".global _main\n");
        printf("_main:\n");
#endif
    }

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", prog->stack_size);

    for (Node *n = prog->body; n; n = n->next) {
        gen_stmt(n);
        assert(depth == 0);
    }

    // Epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");

    if (emu_mode)
        printf("  jmp 0\n");
    else
        printf("  ret\n");
}
