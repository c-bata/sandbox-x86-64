#ifndef VIRTUAL_MEMORY_H_
#define VIRTUAL_MEMORY_H_

#include <stdint.h>
#include <stdio.h>

struct VirtualMemory_t;
typedef struct VirtualMemory_t VirtualMemory;

VirtualMemory* vm_init();
void vm_fread(VirtualMemory* vm, uint64_t vmaddr, FILE* stream, size_t size);
void vm_memcpy(VirtualMemory* vm, uint64_t vmaddr, void* src, size_t size);

uint64_t vm_get_memory8(VirtualMemory* vm, uint64_t vmaddr);
uint64_t vm_get_memory32(VirtualMemory* vm, uint64_t vmaddr);
uint64_t vm_get_memory64(VirtualMemory* vm, uint64_t vmaddr);

void vm_set_memory8(VirtualMemory* vm, uint64_t vmaddr, uint8_t val);
void vm_set_memory32(VirtualMemory* vm, uint64_t vmaddr, uint32_t val);
void vm_set_memory64(VirtualMemory* vm, uint64_t vmaddr, uint64_t val);

#endif
