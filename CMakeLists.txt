cmake_minimum_required(VERSION 3.10)
project(9cc C)

set(CMAKE_C_STANDARD 11)

add_executable(9cc cc/main.c cc/codegen.c cc/parse.c cc/tokenize.c)
add_executable(cpu cpu/main.c cpu/instruction.c cpu/emulator_function.c cpu/modrm.c cpu/io.c
        cpu/virtual_memory.c cpu/macho_loader.c cpu/elf_loader.c)

add_executable(tokendump cc/tools/tokendump.c)
add_executable(astdump cc/tools/astdump.c)
