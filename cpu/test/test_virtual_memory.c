#include <assert.h>
#include "../emulator.h"
#include "../virtual_memory.h"

int main() {
    VirtualMemory* vm = vm_init();
    vm_set_memory8(vm, 0xfffffff8, 0x10);
    uint64_t val = vm_get_memory8(vm, 0xfffffff8);
    assert(val == 0x10);
    return 0;
}
