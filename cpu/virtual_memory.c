#include <stdlib.h>
#include <string.h>
#include "virtual_memory.h"

#define PAGE_SIZE 4096
#define MAX_PAGES 1024

typedef struct {
    uint8_t offset;
    uint8_t* buffer;
} Page;

struct VirtualMemory_t {
    // The size of page table is 16MB to allocate 8GB virtual memory,
    // if page size is 4096 bytes.
    Page* pages[MAX_PAGES];
    uint64_t num_pages;
};

VirtualMemory* vm_init() {
    VirtualMemory* vm = malloc(sizeof(VirtualMemory));
    for (int i = 0; i<MAX_PAGES; i++) {
        vm->pages[i] = NULL;
    }
    vm->num_pages = 0;
    return vm;
}

Page* get_page(VirtualMemory* vm, uint32_t vmaddr) {
    uint16_t offset = vmaddr / PAGE_SIZE;
    // Find page
    for (int i=0; i<vm->num_pages; i++) {
        if (offset == vm->pages[i]->offset) {
            return vm->pages[i];
        }
    }
    // Cache miss. Allocate memory buffer.
    vm->pages[vm->num_pages] = malloc(sizeof(Page));
    vm->pages[vm->num_pages]->offset = offset;
    vm->pages[vm->num_pages]->buffer = malloc(sizeof(uint8_t) * PAGE_SIZE);
    return vm->pages[vm->num_pages++];
}

void vm_fread(VirtualMemory* vm, uint64_t vmaddr, FILE* src, size_t size) {
    uint64_t pos_start = vmaddr;
    uint64_t pos_end = vmaddr + size;

    size_t n_bytes;
    while (size > 0) {
        Page* page = get_page(vm, pos_start);
        uint64_t pos_page_end = (pos_start / PAGE_SIZE +1) * PAGE_SIZE;

        if (pos_end >= pos_page_end) {
            n_bytes = pos_page_end - pos_start;
        } else {
            n_bytes = pos_end - pos_start;
        }
        uint16_t page_offset = pos_start % PAGE_SIZE;
        fread(page->buffer + page_offset, 1, n_bytes, src);
        size -= n_bytes;
    }
}

void vm_memcpy(VirtualMemory* vm, uint64_t vmaddr, void* src, size_t size) {
    uint64_t pos_start = vmaddr;
    uint64_t pos_end = vmaddr + size;

    size_t n_bytes;
    while (size > 0) {
        Page* page = get_page(vm, pos_start);
        uint64_t pos_page_end = (pos_start / PAGE_SIZE +1) * PAGE_SIZE;

        if (pos_end >= pos_page_end) {
            n_bytes = pos_page_end - pos_start;
        } else {
            n_bytes = pos_end - pos_start;
        }
        uint16_t page_offset = pos_start % PAGE_SIZE;
        memcpy(page->buffer + page_offset, src, n_bytes);
        size -= n_bytes;
    }
}

uint64_t vm_get_memory8(VirtualMemory* vm, uint64_t addr) {
    Page* page = get_page(vm, addr);
    uint16_t pos = addr % PAGE_SIZE;
    uint8_t v = page->buffer[pos];
    return v;
}

uint64_t vm_get_memory32(VirtualMemory* vm, uint64_t addr) {
    int i;
    uint32_t ret = 0;
    for (i=0;i<4; i++) {  // little endian
        ret |= vm_get_memory8(vm, addr+i) << (i * 8);
    }
    return ret;
}

uint64_t vm_get_memory64(VirtualMemory* vm, uint64_t addr) {
    int i;
    uint64_t ret = 0;
    for (i=0;i<8; i++) {  // little endian
        ret |= vm_get_memory8(vm, addr+i) << (i * 8);
    }
    return ret;
}

void vm_set_memory8(VirtualMemory* vm, uint64_t addr, uint8_t val) {
    Page* page = get_page(vm, addr);
    uint16_t pos = addr % PAGE_SIZE;
    page->buffer[pos] = val & 0xFF;
}

void vm_set_memory32(VirtualMemory* vm, uint64_t addr, uint32_t val) {
    int i;
    for (i = 0; i<4; i++) {
        vm_set_memory8(vm, addr+i, val >> (i*8));
    }
}

void vm_set_memory64(VirtualMemory* vm, uint64_t addr, uint64_t val) {
    int i;
    for (i = 0; i<8; i++) {
        vm_set_memory8(vm, addr+i, val >> (i*8));
    }
}
