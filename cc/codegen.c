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

static int gen_unique_num(void) {
    static int i = 1;
    return i++;
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
        case ND_FUNCALL:
            printf("  mov rax, 0\n");
#ifdef __linux
            printf("  call %s\n", node->funcname); // ret -> rax
#else
            printf("  call _%s\n", node->funcname); // ret -> rax
#endif
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
            printf("  jmp .L.return\n");
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
    printf(".L.return:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");

    if (emu_mode)
        printf("  jmp 0\n");
    else
        printf("  ret\n");
}
