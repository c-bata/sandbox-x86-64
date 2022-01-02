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

static void gen_expr(Node* node) {
    if (node->kind == ND_NUM) {
        printf("  mov rax, %d\n", node->val);
        return;
    } else if (node->kind == ND_NEG) {
        gen_expr(node->lhs);
        printf("  neg rax\n");
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

void codegen(Node *node, CodeGenOption* option) {
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

    gen_expr(node);

    if (emu_mode)
        printf("  jmp 0\n");
    else
        printf("  ret\n");

    assert(depth == 0);
}
