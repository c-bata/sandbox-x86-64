# This file is created just for IntelliJ CLion.
# I do not use cmake to build executables.

cmake_minimum_required(VERSION 3.10)
project(sandbox-x86-64 C)

set(CMAKE_C_STANDARD 11)

# for C compiler
add_executable(9cc cc/main.c cc/codegen.c cc/parse.c cc/tokenize.c cc/type.c)
add_executable(tokendump cc/tools/tokendump.c)
add_executable(astdump cc/tools/astdump.c)

# for CPU emulator
add_executable(cpu cpu/main.c cpu/instruction.c cpu/emulator_function.c cpu/modrm.c cpu/io.c
        cpu/virtual_memory.c cpu/macho_loader.c cpu/elf_loader.c)
